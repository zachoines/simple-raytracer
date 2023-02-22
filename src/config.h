#pragma once
#include <string>

#define MIN_PIXEL_VALUE 0
#define MAX_PIXEL_VALUE 255
#define PI 3.14159265
#define d 5.0 // Can be any number
#define num_commands 7
#define num_obj_types 4
const std::string valid_commands[] = {"eye", "viewdir", "updir", "hfov", "imsize", "bkgcolor", "mtlcolor"};
const std::string object_types[] = {"sphere", "light", "v", "f"}; // Object is anything with multiple instances
