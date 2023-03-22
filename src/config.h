#pragma once
#include <string>
#include <vector>

// Constants
#define MIN_PIXEL_VALUE 0
#define MAX_PIXEL_VALUE 255
#define d 5.0 // Can be any number
#define num_commands 8
#define num_obj_types 6
#define M_PI 3.14159265358979323846

// Type definitions
typedef unsigned char byte;

// Value-Defintions of the different String values
enum ArgValues {  
    eye,
    viewdir,
    updir,
    hfov,
    imsize,
    bkgcolor,
    mtlcolor,
    texture,
    sphere,
    light,
    v,
    vn,
    vt,
    f
};

// Map to associate the strings with the enum values
std::map<std::string, ArgValues> argsStringValues = {
    {"eye" , eye},
    {"viewdir", viewdir},
    {"updir", updir}, 
    {"hfov", hfov}, 
    {"imsize", imsize}, 
    {"bkgcolor", bkgcolor}, 
    {"mtlcolor", mtlcolor}, 
    {"texture", texture},
    {"sphere", sphere}, 
    {"light", light}, 
    {"v", v}, 
    {"vn", vn}, 
    {"vt", vt}, 
    {"f", f}
};
