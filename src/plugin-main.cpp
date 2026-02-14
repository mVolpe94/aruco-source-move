/*
Plugin Name
Copyright (C) <Year> <Developer> <Email Address>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <plugin-support.h>
#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <opencv2/aruco.hpp>
#include <media-io/video-scaler.h>
#include <sstream>
#include <string>

using namespace cv;

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

const char *FILTER_NAME = "ArUco Source Move";


template <typename T>
void log_var(const char* name, const T& value)
{
    std::ostringstream oss;
    oss << name << " = " << value;
    blog(LOG_INFO, "%s", oss.str().c_str());
}


struct aruco_data {
    // self reference
    obs_source_t *source;
    obs_source_t *selected_source;
    obs_sceneitem_t *scene_item;
    int source_w;
    int source_h;

    // settings
    int aruco_id;
    bool draw_marker;

    // aruco dict
    cv::Ptr<cv::aruco::Dictionary> dictionary;

    // video conversion for opencv
    video_scaler_t *scaler_simple;

    // last marker position in case lost detection
    double last_x, last_y;
    uint32_t frame_counter;
    uint8_t skip;

    // marker state
    bool marker_visible;
    double mark_x;
    double mark_y;
    double mark_rotation;
    double mark_size;
    bool transform_dirty;

    // scaling factor
    double scaling_factor;
};


//Runs every OBS tick to move the source on the screen when ArUco marker is found
static void tick_callback(void *data, float seconds)
{
    UNUSED_PARAMETER(seconds); 
    struct aruco_data *filter = (aruco_data *)data;

    if (!filter->transform_dirty || !filter->marker_visible)
        return;

    if (!filter->selected_source || !filter->scene_item)
        return;

    struct vec2 pos;
    pos.x = (float)filter->mark_x;
    pos.y = (float)filter->mark_y;

    struct vec2 orig_size;
    orig_size.x = (float)filter->source_w;
    orig_size.y = (float)filter->source_h;

    struct vec2 obs_scale_factor;
    obs_scale_factor.x = (float)filter->mark_size / orig_size.x;
    obs_scale_factor.y = (float)filter->mark_size / orig_size.x;

    obs_scale_factor.x += (float)filter->scaling_factor; 
    obs_scale_factor.y += (float)filter->scaling_factor;

    if (obs_scale_factor.x < 0 || obs_scale_factor.y < 0) {
        obs_scale_factor.x = 0;
        obs_scale_factor.y = 0;
    }

    obs_sceneitem_set_pos(filter->scene_item, &pos);
    obs_sceneitem_set_scale(filter->scene_item, &obs_scale_factor);
    obs_sceneitem_set_rot(filter->scene_item, (float)filter->mark_rotation);

    filter->transform_dirty = false;

    return;
}


static void filter_update(void *data, obs_data_t *settings);


//Simply returns the filter name
const char *get_filter_name(void *unused)
{
    UNUSED_PARAMETER(unused);
    return FILTER_NAME;
}


//This function grabs the scene item from a specified source and adds it to the filter struct
static bool find_scene_item(obs_scene_t *scene, obs_sceneitem_t *item, void *data)
{
    aruco_data *filter = (aruco_data *)data;
    obs_source_t *src = obs_sceneitem_get_source(item);

    if (src == filter->selected_source) {
        filter->scene_item = item;
        return false;
    }
    return true;
}


//Only runs when filter is added to a source
static void *filter_create(obs_data_t *settings, obs_source_t *source)
{
    struct aruco_data *filter =
        (struct aruco_data *)bzalloc(sizeof(struct aruco_data));
    
    filter->source = source;
    filter->selected_source = NULL;
    filter->scene_item = NULL;

    obs_source_t *scene_source = obs_frontend_get_current_scene();
    obs_scene_t *scene = obs_scene_from_source(scene_source);
    const char *source_name = obs_data_get_string(settings, "source_name");
    filter->selected_source = obs_get_source_by_uuid(source_name);

    if (scene && filter->selected_source) {
        obs_scene_enum_items(scene, find_scene_item, filter);
    }

    filter->dictionary = cv::makePtr<cv::aruco::Dictionary>(cv::aruco::getPredefinedDictionary(cv::aruco::DICT_4X4_50));
    filter->draw_marker = false;
    filter->aruco_id = 0;
    filter->scaler_simple = NULL;
    filter->last_x = 0.0;
    filter->last_y = 0.0;
    filter->frame_counter = 0;
    filter->skip = 0;
    filter->scaling_factor = 0.00;
    filter->marker_visible = false;
    filter->mark_x = 0.0;
    filter->mark_y = 0.0;
    filter->mark_size = 0.0;
    filter->mark_rotation = 0.0;

    obs_add_tick_callback(tick_callback, filter);
    filter_update(filter, settings);
    obs_source_update(source, settings);

    obs_source_release(scene_source);

    return filter;
}


static void filter_destroy(void *data)
{
    struct aruco_data *filter = (struct aruco_data *)data;
    if (filter->scaler_simple)
        video_scaler_destroy(filter->scaler_simple);
    if (filter->selected_source)
        obs_source_release(filter->selected_source);
    if (filter->source)
        obs_source_release(filter->source);
    obs_remove_tick_callback(tick_callback, filter);
    bfree(filter);
}


static struct obs_source_frame *filter_video(void *data, struct obs_source_frame *frame)
{
    struct aruco_data *filter = (struct aruco_data *)data;

    filter->frame_counter++;
    if (filter->skip > 0 && filter->frame_counter < filter->skip) {
        return frame;
    }
    filter->frame_counter = 0;

    if (filter->scaler_simple == NULL) {
        struct video_scale_info origin;
        origin.format = frame->format;
        origin.width = frame->width;
        origin.height = frame->height;
        origin.range = frame->full_range ? VIDEO_RANGE_FULL : VIDEO_RANGE_PARTIAL;
        origin.colorspace = VIDEO_CS_DEFAULT;

        struct video_scale_info dest;
        dest.format = VIDEO_FORMAT_Y800;
        dest.width = frame->width;
        dest.height = frame->height;
        dest.range = VIDEO_RANGE_FULL;
        dest.colorspace = VIDEO_CS_DEFAULT;

        int is_scaler_created = video_scaler_create(&filter->scaler_simple, &origin, &dest, VIDEO_SCALE_DEFAULT);
        obs_log(LOG_INFO, "ArUco Source Move: video_scaler_create scaler_simple returned %d", is_scaler_created);
    }

    struct obs_source_frame *scaled_frame = obs_source_frame_create(VIDEO_FORMAT_Y800, frame->width, frame->height);
    scaled_frame->timestamp = frame->timestamp;

    bool is_video_scaled = video_scaler_scale(filter->scaler_simple, scaled_frame->data, scaled_frame->linesize, frame->data, frame->linesize);

    if (!is_video_scaled) {
        obs_log(LOG_ERROR, "ArUco Source Move: video_scaler_scale failed");
    }

    cv::Mat image(frame->height, frame->width, CV_8UC1, scaled_frame->data[0]);

    if (image.data != NULL) {
        std::vector<int> ids;
        std::vector<std::vector<cv::Point2f>> corners;

        cv::aruco::detectMarkers(image, filter->dictionary, corners, ids);

        if (ids.size() > 0) {

            for (size_t i = 0; i < ids.size(); i++) {
                if (ids[i] != filter->aruco_id)
                    continue;

                double cx = 0.0, cy = 0.0;
                for (int c = 0; c < 4; c++) {
                    cx += corners[i][c].x;
                    cy += corners[i][c].y;
                }
                cx /= 4.0;
                cy /= 4.0;

                cv::Point2f v = corners[i][1] - corners[i][0];
                double rotation_deg = (atan2(v.y, v.x) * 180 / CV_PI);

                double edge_len = cv::norm(v);
                
                filter->mark_x = cx;
                filter->mark_y = cy;
                filter->last_x = cx;
                filter->last_y = cy;
                filter->mark_rotation = rotation_deg;
                filter->mark_size = edge_len;
                filter->marker_visible = true;
                filter->transform_dirty = true;

                break;
            }
        } else {
            filter->mark_x = filter->last_x;
            filter->mark_y = filter->last_y;
            filter->marker_visible = false;
        }
    } else {
        obs_log(LOG_INFO, "ArUco Source Move: Image data missing or failed to load.");
    }

    obs_source_frame_destroy(scaled_frame);
    return frame;
}



static void filter_activate(void *data)
{
    struct aruco_data *filter = (aruco_data *)data;
    obs_data_t *settings = obs_source_get_settings(filter->source);

    const char *source_name = obs_data_get_string(settings, "source_name");

    obs_source_t *scene_source = obs_frontend_get_current_scene();
    obs_scene_t *scene = obs_scene_from_source(scene_source);

    filter->selected_source = obs_get_source_by_uuid(source_name);
    filter->source_h = obs_source_get_height(filter->selected_source);
    filter->source_w = obs_source_get_width(filter->selected_source);

    if (scene && filter->selected_source) {
        obs_scene_enum_items(scene, find_scene_item, filter);
    }

    obs_data_release(settings);
    obs_source_release(scene_source);
}


static void filter_update(void *data, obs_data_t *settings)
{
    struct aruco_data *filter = (struct aruco_data *)data;
    
    const char *source_name = obs_data_get_string(settings, "source_name"); 

    blog(LOG_INFO, source_name);

    filter->scene_item = NULL;
    obs_source_t *scene_source = obs_frontend_get_current_scene();
    obs_scene_t *scene = obs_scene_from_source(scene_source);

    int id = (int)obs_data_get_int(settings, "aruco_id");
    bool draw_marker = obs_data_get_int(settings, "draw_marker");
    int skip_frames = (int)obs_data_get_int(settings, "skip_frames");

    double scaling_factor = obs_data_get_double(settings, "scaling_factor");

    filter->selected_source = obs_get_source_by_uuid(source_name);

    if (filter->selected_source == NULL)
        blog(LOG_INFO, "Selected Source NULL");

    if (scene && filter->selected_source) {
        obs_scene_enum_items(scene, find_scene_item, filter);
    }

    filter->source_h = obs_source_get_height(filter->selected_source);
    filter->source_w = obs_source_get_width(filter->selected_source);
    filter->aruco_id = id;
    filter->draw_marker = draw_marker;
    filter->skip = skip_frames;
    filter->scaling_factor = scaling_factor;

    obs_source_release(scene_source);
}


static bool add_scene_item_to_list(obs_scene_t *scene, obs_sceneitem_t *item, void *data){
    obs_property_t *list = (obs_property_t*)data;

    obs_source_t* source = obs_sceneitem_get_source(item);

    if (!source)
        return true;
    
    if (obs_source_get_type(source) == OBS_SOURCE_TYPE_FILTER)
        return true;

    const char* uuid = obs_source_get_uuid(source);
    const char* name = obs_source_get_name(source);

    if (!uuid || !name)
        return true;

    obs_property_list_add_string(list, name, uuid);

    return true;
}


static obs_properties_t *filter_properties(void *data)
{
    struct aruco_data *filter = (struct aruco_data *)data;

    obs_source_t *parent = obs_frontend_get_current_scene();
    obs_scene_t *scene = obs_scene_from_source(parent);

    obs_properties_t *props = obs_properties_create();

    obs_properties_t *group = obs_properties_create();
    obs_property_t *p = obs_properties_add_list(group, "source_name", obs_module_text("Source"), OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
    obs_property_list_add_string(p, "None", "");
    obs_scene_enum_items(scene, add_scene_item_to_list, p);
    p = obs_properties_add_group(props, "general_group", "General", OBS_GROUP_NORMAL, group);

    group = obs_properties_create();
    obs_properties_add_int(group, "aruco_id", "ArUco ID", 0, 49, 1);
    obs_properties_add_bool(group, "draw_marker", "Draw Marker");
    obs_properties_add_int(group, "skip_frames", "Skip Frames", 0, 60, 1);
    obs_properties_add_group(props, "aruco_group", "ArUco Settings", OBS_GROUP_NORMAL, group);
    
    group = obs_properties_create();
    
    obs_properties_add_float_slider(group, "scaling_factor", "Scaling Factor", -1.00, 1.00, 0.01);
    obs_properties_add_group(props, "scaling_group", "Scaling Settings", OBS_GROUP_NORMAL, group);

    obs_source_release(parent);

    return props;
}


extern "C" struct obs_source_info filter_info = {
    .id = "aruco_source_move",
    .type = OBS_SOURCE_TYPE_FILTER,
    .output_flags = OBS_SOURCE_ASYNC_VIDEO,
    .get_name = get_filter_name,
    .create = filter_create,
    .destroy = filter_destroy,
    .get_properties = filter_properties,
    .update = filter_update,
    .activate = filter_activate,
    .filter_video = filter_video,
};


bool obs_module_load(void)
{
    obs_register_source(&filter_info);

	obs_log(LOG_INFO, "plugin loaded successfully (version %s)", PLUGIN_VERSION);

	return true;
}

void obs_module_unload(void)
{
	obs_log(LOG_INFO, "ArUco Source Move: Plugin unloaded.");
}

