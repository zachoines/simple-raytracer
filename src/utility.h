#pragma once
#include <iostream>
#include <string>
#include "definitions.h"

bool objectInStack(std::vector<SceneObjectInfo*> &object_stack, SceneObjectInfo* object) {
    if (std::find(object_stack.begin(), object_stack.end(), object) != object_stack.end()) {
        return true;
    } else {
        return false;
    }
}

/*
    Transforms an input from range 1 to range 2

    x Input
    in_min Minimum value or input range
    in_max Maximum value or input range
    out_min Minimum value or output range
    out_max Maximum value or output range 
*/
float map(float x, float in_min, float in_max, float out_min, float out_max) 
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

/*
    Removes the extension from a string path. Removes in place.
    ex.) path.txt => path

    path String to modify
*/
void remove_extension(std::string &path)
{
    std::size_t dot = path.rfind(".");
    if (dot != std::string::npos)
    {
        path.resize(dot);
    }
}

/*
    Note: Only support PPM with 'P3' image file format. Image height and width must be 512 and 512.
    Range of values must be between 0 to 255.

    Example PPM header/body:

    P3
    # Comment
    512 512
    255
    127 178 229  127 178 229 ...
    127 178 229  127 178 229 ... 
    127 178 229  127 178 229 ...
    127 178 229  127 178 229 ...
    ...
*/
Texture* read_texture(std::string path, Texture* texture) {
    unsigned int current_line = 0;
    unsigned int current_token = 0;
    int height, width;
    std::string line;
    std::ifstream input_file(path);
    std::vector<std::string> tokens;

    if (input_file.is_open()) 
    {
        // Read in one line at a time
        while (std::getline(input_file, line))
        {
            current_line++;
            std::istringstream ss(line);
            std::string del;

            // Skip comments
            if (line.at(0) == '#') {
                continue;
            }

            // Strip line of whitespace
            while(std::getline(ss, del, ' ')) {
                
                // If not blank string or comment
                if (!del.empty()) {
                    current_token++;
                    if (current_token == 1) {
                        if (del != "P3") {
                            throw std::invalid_argument("Only supports PPM 'P3' file format.");
                        }
                    } 
                    else if (current_token == 2) 
                    {
                        width = std::stoi(del);
                    }
                    else if (current_token == 3) 
                    {
                        height = std::stoi(del);
                    }
                    else if (current_token == 4) 
                    {
                        if (del != "255") {
                            throw std::invalid_argument("PPM pixel value must be between 0 - 255 .");
                        }
                    }
                    else 
                    {
                        tokens.push_back(del);
                    }
                }
            }
        }
    }

    texture = new Texture(width, height);

    // Now write to image
    unsigned int token_index = 0;
    int i, j, k;
    try
    {
        
        for (j = 0; j < height; j++ ) {
            for (i = 0; i < width; i++ ) {
                for (k = 0; k < 3; k++ ) {
                    texture->image->set(i, j, k, std::stoi(tokens[token_index]));
                    token_index += 1;
                }
            }
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        throw e;
    }
    
    return texture;
}