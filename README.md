# SolarSystem2
Live solar system animation created with OpenGL

Virtual Orery and Real Time Rendering

![Solar System Preview](https://github.com/juandiegovil/SolarSystem2/assets/66028457/5c30b213-c374-42e3-8774-3987fad06fa1)


This program produces a simulation of the Solar System. The planets/moons do their respective orbits at some orbit angle. The planets, moons and the sun all have an axis of rotation relative to their orbit, they'll rotate on that axis at some speed relative to the actual time the system has been running for (not framerate). Shading based on position was also added to each planet/moon

## Running Program:
Open the 453-skeleton.exe file in \SolarSystem\out\build\x64-Debug folderpath

## Controls:
Camera:
	Scroll wheel zooms in and out on the cube
	Holding the right mouse button and dragging allows you to rotate the camera around the cube
Animations:
	Speed - Tap or hold the RIGHT ARROW KEY to speed up the animation, LEFT ARROW KEY to slow down the animation
	Restart - Tap the R KEY to restart the animation (previous speed will hold)
	Pause - Use the SPACEBAR to toggle between pause and play

## Extra Notes:
	The size, tilt angle, rotating speed, orbit angle, and orbiting speed of each planet are approximately accurate (relative to earths properties) to the real     
   world. Only a maximum of 3 moons were added for per planet.
	To change the shineiness coefficient or the strength of the specular, diffuse or ambient reflections/light go to the test.frag and the respective variables can 
   be seen defined there.
	In the window you can see some thin lines added for the x, y and z axis just for easiness of vizualisation and testing .


## Compiler and Platform
Compiler: Clang++
Platform: Microsoft Windows 11, 64bit
Build Tools: Cmake
Application: Microsoft Visual Studio 2022
