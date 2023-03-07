#pragma once
#include <string>
#include <vector>

// Constants
#define MIN_PIXEL_VALUE 0
#define MAX_PIXEL_VALUE 255
#define d 5.0 // Can be any number
#define num_commands 8
#define num_obj_types 6
# define M_PI 3.14159265358979323846

// Valid arguements for program
const std::string valid_commands[] = {"eye", "viewdir", "updir", "hfov", "imsize", "bkgcolor", "mtlcolor", "texture"};
const std::string object_types[] = {"sphere", "light", "v", "vn", "vt", "f"}; // Object is anything with multiple instances

// Type definitions
typedef unsigned char byte;