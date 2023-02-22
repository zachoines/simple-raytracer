#pragma once
#include <string>

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