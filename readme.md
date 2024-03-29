# Overview
This program is a raytracing based renderer that generates images using a properties file as input. This is a work in progress. 
Currently supported features are: 
- Reflections
- Translucency
- Hard shadows
- Point and directional lights
- Phong Illumination Materials 
- Spheres
- Triangles (faces)
- Textures


![Example](Examples/demo_image.png)

# Compile program
First navigate to directory with main.cpp. Then run these commands:  
- cmake .
- cmake --build ./

On windows, this will create 'Debug' directory with 'SimpleRayTracer.exe' within.

# Using program

Drag and drop a config file like "scene_simple.txt" or "scene_complex.txt" into the same directory as the executable.

You can run the program like so:
- .\raytracer1b.exe .\rubber_eraser.txt

Valid arguements for config files include:
- eye eyex eyey eyez
    - The location of the 'eye' within scene
- viewdir vdirx  vdiry  vdirz                
    - Defines the direction the 'eye' is looking
- updir upx  upy  upz
    - Roll of the camera
- hfov fovh
    - Horizonal field of view
- imsize width  height
    - Output image dimentions
- bkgcolor r  g  b  η
    - Scene background color. Param η is optional 
- mtlcolor Od Od Od Os Os Os ka kd ks n α η 
    - Material color. Params α η are optional
- texture texture.ppm
    - Texture to apply to model          
- light x y z w r g b
    - Scene light. Directional or point.
- sphere cx  cy  cz  r                       
    - Sphere defined by center and radiusm
- vn nx ny nz
    - Vertex normal
- vt tx ty
    - texture coordinates
- \#
    - Single line comment
- f v1/vt1/vn1 v2/vt2/vn2 v3/vt2/vn
    - smooth-shaded, textured triangle
- f v1//vn1 v2//vn2 v3//vn3
    - (smooth-shaded, untextured triangle)
- f v1/vt1 v2/vt2 v3/vt2
    - non-smooth-shaded, textured triangle

# Configure Debugging on Windows
Follow tutorial to install GNU C++ on windows:
https://www.youtube.com/watch?v=rgCJbsCSARM&ab_channel=LearningLad

1. First download and install MinGW-w64 for windows
    https://www.mingw-w64.org/downloads/#msys2

2. Install compiler and debugger for C/C++
    * open MSYS
        * Update the package database and base packages using
        pacman -Syu

        * Update rest of the base packages 
        pacman -Su

    * open MSYS2 MinGW 64-bit
        * To install compiler ( gcc ) for C and C++
            - Print all available packages
                pacman -Ss gcc 

            - In the output you will find one for C/C++
                mingw64/mingw-w64-x86_64-gcc 12.2.0-9 (mingw-w64-x86_64-toolchain)
                GNU Compiler Collection (C,C++,OpenMP) for MinGW-w64

            - Now install GCC
                pacman -S mingw-w64-x86_64-gcc

            - Check if its installed
                gcc --version
                g++ --version

        * To install the debugger ( gdb ) for C and C++
            - Print all available gdb version
                pacman -Ss gdb 
            
            - Install correct version
                pacman -S mingw-w64-x86_64-gdb

            - Check if its installed
                gdb --version

3. Set your environmental variables
    Add C:\msys64\mingw64\bin to your "path" variable

4. Set up build environment in visual studio
    * Configure c_cpp_properties.json
        * Open command pallete
            run "C/C++: Edit Configurations (UI)"
        * Set compiler 
            - IntelliSense mode "gcc-x86"
            - Compiler Path "C:\msys64\mingw64\bin\gcc.exe"

5. Now configure a C/C++ build task
    * This will create a new task
        In the Menu bar of visual studio code 
        Terminal -> Configure Default Build Task -> C/C++: g++.exe build active file
        
    * Open and edit the generated task
        under args add: 
            "-g3" (for debugging with max info. Don't use -g)
            "-Wall" (warnings) 

6. Build and run C/C++ file
    * Run default build task (One created prior)
        Ctrl + Shift + B (Generates main.exe)
    * Run the executable 
        * open command prompt
        * Navigate to directory
        * Run "main.exe"

7. Set up debugger
    * Install visual studio code
    * In VSCode, install C/C++ plugin
    * Configure a launch.json file, with a C/C++ launch entry
    * Add replace miDebuggerPath with "C:\\msys64\\mingw64\\bin\\gdb.exe",
