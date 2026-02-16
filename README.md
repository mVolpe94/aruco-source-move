# ArUco Source Move OBS Plugin

## Introduction

This plugin gives OBS the functionality to detect ArUco markers using OpenCV plus Contrib Module: ArUco. Apply the filter to any video source and map a user chosen source within the same scene to the position, size and rotation of a specific detected ArUco marker. 

Left side, plugin inactive, right side plugin active:
![Example Video](images/readmegifbig.gif)

The files include:
* Plugin Source code (src folder)
* CMakeLists.txt to build (requires OpenCV and Contrib)

## Supported Build Environments

| Platform | Tool                  |
| -------- | --------------------- |
| Windows  | Visual Studio 17 2022 |
| Windows  | CMake 4.2.2           |
## Using the Plugin

1. Install plugin by either running the installer 
	- Instead of running the installer, you may just place the "aruco-source-move.dll" and "aruco-source-move.pdb" files to the directory that your copy of OBS looks for plugins.
2. Open OBS
3. On a video source you would like to track ArUco markers open the filters menu and add the ArUco Source Move filter to the Audio/Video Filters section.
4. Choose a source for the filter to act upon (This should be the source you want to move around to the ArUco marker)
	- NOTE: This menu only finds sources within the same scene as the video source that the filter is placed in.
    - NOTE: The source you choose MUST be set to have a center positional alignment.
5. Update the ArUco ID to match the specific ArUco marker ID that you are planning to track.
	- Look [here](https://www.geeksforgeeks.org/computer-vision/detecting-aruco-markers-with-opencv-and-python-1/)  or [here](https://docs.opencv.org/4.x/d5/dae/tutorial_aruco_detection.html) for more info
	- Go [here](https://chev.me/arucogen/) to generate ArUco markers
6. Set the next settings as you prefer
	- "Show source only when marker is detected" will toggle the visibility of the selected source off and on when the ArUco marker is either detected or not
	- "Skip Frames" will skip frames of detection to cut down on resources
	- "Scaling Factor" allows you to change the size of the selected source relative to the size of the ArUco marker. If you don't want to see the marker behind your source, you can make the source bigger to compensate.

## How to Build

To build this plugin, you will need a working build of OpenCV with Contrib Modules. Building OpenCV without the Contrib Modules will not provide you the libraries needed to detect ArUco markers. This was a recent change where ArUco detection was moved out of the main OpenCv library and into the Contrib Modules. 

**Steps:**
1. Download latest OpenCV (4.12.0) Source Code from [here](https://opencv.org/releases/)
2. Clone or download the latest Master branch of the OpenCV Contrib Modules found [here](https://github.com/opencv/opencv_contrib)
3. Download and install the latest version of CMake (4.2.2) found [here](https://cmake.org/download/)
4. Place the opencv-4.12.0.zip and opencv_contrib-master.zip files within any "easy for you to access" directory
	- I placed mine within a folder I made "C:\opencv" for convenience
5. Unzip opencv-4.12.0 into your opencv folder and rename the folder to "source"
	- You can name it whatever you like, but let's call it what it is so it makes sense when we open CMake's GUI
6. Create a folder called "build" within your opencv directory
7. Unzip opencv_contrib-master into your opencv folder as well
8. Open CMake 
9. Set the source location to your opencv/source folder
10. Set the build location to your empty opencv/build folder
11. Click configure
12. Set the compiler you want to use
	- I used VS 18 2026
13. The middle box should populate with a list
	- Everything in red is new since the last configure
14. Find the EXTRAS path option in the list and set it to: opencv/opencv_contrib-modules/modules
15. Configure a second time
16. There should be new list items populated in red
17. Make sure the modules you want are checked
	- For this project, make sure that BUILD_opencv_aruco is checked
	- If you are using OpenCV for non-commercial use, you can check the enable nonfree modules option as well (should not affect building this plugin)
18. Configure one last time
19. Click Generate
20. Open Project
21. Right click on the .sln file and build
	- Build the debug and release versions
	- This takes a while
22. Right click INSTALL and build
	- Build the debug and release versions
	- This takes a while
23. Set Windows Path Variable
	- Set 3 PATH variables:
		1. "..opencv/build/bin/release"
		2. "..opencv/build/lib/release"
		3. "..opencv/build/install/include/opencv2"
	- **This is how my folder setup was after building OpenCV. Almost every other tutorial I found on how to set this up had a different folder structure than I ended up with. So, as long as you find where the libraries were built to, where the dlls were build to, and where the opencv2 folder build to. It should work.**
24. Clone this repository and in your IDE of choice, build the CMakeLists.txt file
25. Find the aruco-source-move.dll and aruco-source-move.pdb inside the build_x64 that should have been created and place those two files inside the plugins folder for OBS.

## Things to Keep in Mind

- This cannot be used to determine the tilt of the ArUco marker, in fact, I have reason to believe that a marker that is being tilted in one direction or another may cause the rotation of the source to not behave as expected. I created this to be used by an ArUco marker that I knew was always vertical flat to the screen.
- CPU usage on this plugin is high due to the use of computer vision (7-11%)
	- I plan to work on ways to cut the usage down. If anyone has any tips, please send me a message!
- If a marker is only half on screen or obstructed, detection may not work as intended
- If you have any questions or suggestions, please reach out!

## Potential Use Cases

I made this plugin so that I could create cool alerts for my Twitch stream! I was inspired by Nutty's wild alerts that he used extensive chains of Move plugin filters to achieve. What I planned to do was create something similar that would run when someone raids me. There will be a video playing and my profile picture is on one character's face and I wanted my raider's profile picture to be on the other.

![Example Video](images/readmegifbig.gif)

So, I made a video with an ArUco marker for my raider's face (left side) that through use of this plugin, will be replaced by a browser source linked to my raider's profile picture (right side). Something to make my raider feel special and make their community have fun!

**Other Ideas:**
- In a "Just Chatting Scene" you may use this plugin to use sources as "props" that you can manipulate and move around
- Streamer wears an ArUco marker on their body and their chat can use channel point redeems to change what is overlayed on the marker
- It may assist VTubers tracking their model.
- With Streamerbot integration, there may be a way to make scene changes simply by holding up printed ArUco markers.
- If you think of any let me know, I would love to see how this plugin could be useful for you!
