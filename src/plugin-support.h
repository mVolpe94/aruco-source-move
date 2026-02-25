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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

extern const char *PLUGIN_NAME;
extern const char *PLUGIN_VERSION;

void obs_log(int log_level, const char *format, ...);
extern void blogva(int log_level, const char *format, va_list args);

#define MAX_HALF_LIFE_POS 0.2 // seconds
#define MAX_HALF_LIFE_ROT 0.2 // seconds
#define MAX_HALF_LIFE_SCALE 0.2 // seconds
#define MAX_SCALING_FACTOR 10.0 // maximum scaling factor for slider
#define MAX_EASING_FACTOR 4.0 // maximum easing factor for sliders
#define DEFAULT_EASING_FACTOR 0.20 // default easing factor for position, rotation, and scaling

const double SLIDER_GRANULARITY = 0.01; // slider step size

// Properties KEYS
#define SOURCE_NAME "source_name"
#define GENERAL_GROUP "general_group"
#define POSITION_GROUP "position_group"
#define SCALING_GROUP "scaling_group"
#define ROTATION_GROUP "rotation_group"
#define ARUCO_GROUP "aruco_group"
#define ARUCO_ID "aruco_id"
#define SCENEITEM_VISIBILITY "sceneitem_visibility"
#define SKIP_FRAMES "skip_frames"
#define POSITION_EASING_FACTOR "position_easing_factor"
#define ROTATION_EASING_FACTOR "rotation_easing_factor"
#define SCALING_EASING_FACTOR "scaling_easing_factor"
#define SCALING_FACTOR "scaling_factor"


#ifdef __cplusplus
}
#endif
