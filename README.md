# Raycasting-tutorial-series
A series of progressive implementations on ray casting

The first part of this series is based on the Permadi tutorial. The second part comprises my own elaborations on the concept.

The Permadi tutorial can be found at https://permadi.com/1996/05/ray-casting-tutorial-4/

The series is progressive - each next implementation builds upon the previous one. So in order to understand the code, I advise you to start at the first implementation (that is part 9a - these numbers refer to the Permadi tutorial parts), and work your way through the series. It helps of course if you check out the Permadi tutorial while looking through the code.

In each cpp file I added a short description where I summarize what that particular part is about. 

There are a few dependencies:
1. all implementations are built upon the olcPixelGameEngine.h header file by JavidX9 - see: https://github.com/OneLoneCoder/olcPixelGameEngine 
2. for most of the implementations you need some sprite files (.png) for the texturing. Adapt the code to load these sprite files in OnUserCreate() to get it working on your setup.
3. starting with part 23 I isolated map definition info in separate .h files

# Update 
April 6, 2023

After implementing the ray casting tutorial series I decided to elaborate on the functionality of this code. Starting from part 20 I'm currently expanding on the series.

Check out the file Raycasting tutorial - implementations on github YYYYMMDD.pdf to see what has been added.

Have fun with it!

Joseph21
