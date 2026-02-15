# ArUco Source Move OBS Plugin

## Introduction

This plugin gives OBS the functionality to detect ArUco markers using OpenCV plus Contrib Module: ArUco. Apply the filter to any video source and map a user chosen source within the same scene to the position, size and rotation of a specific detected ArUco marker. The files include:

* Plugin Source code (src folder)
* CMakeLists.txt to build (requires OpenCV and Contrib)

## Supported Build Environments

| Platform | Tool                  |
| -------- | --------------------- |
| Windows  | Visual Studio 18 2026 |
| Windows  | CMake 4.2.2           |
## Using the Plugin

1. Install plugin by either running the installer 
	- Instead of running the installer, you may just place the "aruco-source-move.dll" and "aruco-source-move.pdb" files to the directory that your copy of OBS looks for plugins.
2. Open OBS
3. On a video source you would like to track ArUco markers open the filters menu and add the ArUco Source Move filter to the Audio/Video Filters section.
4. Choose a source for the filter to act upon (This should be the source you want to move around to the ArUco marker)
	- NOTE: This menu only finds sources within the same scene as the video source that the filter is placed in.
5. Update the ArUco ID to match the specific ArUco marker ID that you are planning to track.
	- Look [here](https://www.geeksforgeeks.org/computer-vision/detecting-aruco-markers-with-opencv-and-python-1/)  or [here](https://docs.opencv.org/4.x/d5/dae/tutorial_aruco_detection.html) for more info
	- Go [here](https://chev.me/arucogen/) to generate ArUco markers
6. Set the next settings as you prefer
	- "Show source only when marker is detected" will toggle the visibility of the selected source off and on when the ArUco marker is either detected or not
	- "Skip Frames" will skip frames of detection to cut down on resources
	- "Scaling Factor" allows you to change the size of the selected source relative to the size of the ArUco marker. If you don't want to see the marker behind your source, you can make the source bigger to compensate.