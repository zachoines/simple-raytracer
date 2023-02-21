/*
    Follow tutorial to install GNU C++ on windows:
    https://www.youtube.com/watch?v=rgCJbsCSARM&ab_channel=LearningLad

    1.) First download and install MinGW-w64 for windows
        https://www.mingw-w64.org/downloads/#msys2

    2.) Istall compiler and debugger for C/C++
        a.) open MSYS
            1.) Update the package database and base packages using
            pacman -Syu

            2.) Update rest of the base packages 
            pacman -Su

        b.) open MSYS2 MinGW 64-bit
            1.) To install compiler ( gcc ) for C and C++
                
                i.) Print all available packages
                    pacman -Ss gcc 

                ii.) In the output you will find one for C/C++
                    mingw64/mingw-w64-x86_64-gcc 12.2.0-9 (mingw-w64-x86_64-toolchain)
                    GNU Compiler Collection (C,C++,OpenMP) for MinGW-w64

                iii.) Now install GCC
                    pacman -S mingw-w64-x86_64-gcc

                IV.) Check if its installed
                    gcc --version
                    g++ --version

            2.) To install the debugger ( gdb ) for C and C++
        
                i.) Print all available gdb version
                    pacman -Ss gdb 
                
                ii.) Install correct version
                    pacman -S mingw-w64-x86_64-gdb

                iii.) Check if its installed
                    gdb --version

    3.) Set your environmental variables
        Add C:\msys64\mingw64\bin to your "path" variable

    4.) Set up build environment in visual studio
        a.) Configure c_cpp_properties.json
            i.) Open command pallete
                run "C/C++: Edit Configurations (UI)"
            ii.) Set compiler 
                - IntelliSense mode "gcc-x86"
                - Compiler Path "C:\msys64\mingw64\bin\gcc.exe"
    
    5.) Now configure a C/C++ build task
        a.) This will create a new task
            In the Menu bar of visual studio code 
            Terminal -> Configure Default Build Task -> C/C++: g++.exe build active file
            
        b.) Open and edit the generated task
            under args add: 
                "-g3" (for debugging with max info. Don't use -g)
                "-Wall" (warnings) 

    6.) Build and run C/C++ file
        a.) Run default build task (One created prior)
            Ctrl + Shift + B (Generates main.exe)
        b.) Run the executable 
            i.) open command prompt
            ii.) Navigate to directory
            iii.) Run "main.exe"

    7.) Set up debugger
        a.) 

*/
