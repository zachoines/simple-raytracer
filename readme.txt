This program generates a very simple PPM image based on a properties file. 
The outputted image has a black background with a single line across its horizontal. 
The properties file should contain the width and height of the output image, 
following this format on a single line: imsize width height. 
Omitting any one of these will result in the program instructing the user to re-enter.
The path of the input properties file is passed a an argument to the program. 

Example usage:
./main.exe input_desc.txt