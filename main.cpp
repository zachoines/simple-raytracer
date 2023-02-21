#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <numeric>
#include <cmath> 
#include <map>

typedef unsigned char byte;

#define MIN_PIXEL_VALUE 0
#define MAX_PIXEL_VALUE 255
#define PI 3.14159265
#define d 5.0 // Can be any number
#define num_commands 7
#define num_obj_types 1
#define Mat2d std::vector<std::vector<byte>>
#define Mat3d std::vector<Mat2d>

const std::string valid_commands[] = {"eye", "viewdir", "updir", "hfov", "imsize", "bkgcolor", "mtlcolor"};
const std::string object_types[] = {"sphere"};

struct Vector3 {
    float values[3] = { 0.0 };


    float x () {
        return values[0];
    }

    float y () {
        return values[1];
    }

    float z () {
        return values[2];
    }

    float sum() {
        return values[0] + values[1] + values[2];
    } 

    float dot(Vector3 other) {
        return (*this * other).sum();
    }

    float mag() {
        return this->square().sum();
    }

    Vector3 square() {
        Vector3 result = *this;
        result.values[0] *= result.values[0];
        result.values[1] *= result.values[1];
        result.values[2] *= result.values[2];
        return result;
    } 

    Vector3 cross (Vector3 other) {
        Vector3 result;
        result.values[0]=this->y()*other.z() - this->z()*other.y();
        result.values[1]=this->z()*other.x() - this->x()*other.z();
        result.values[2]=this->x()*other.y() - this->y()*other.x();
        return result;
    }
    
    Vector3 norm() {
        float accum = 0.0;
        for (int i = 0; i < 3; i++) {
            accum += values[i] * values[i];
        }
        float mag = std::sqrt(accum);
        
        Vector3 result = *this;
        result.values[0] /= mag;
        result.values[1] /= mag;
        result.values[2] /= mag;
        return result;
    }

    // Op overloads
    Vector3 operator * (float other) {
        Vector3 result = *this;
        result.values[0] *= other;
        result.values[1] *= other;
        result.values[2] *= other;
        return result;
    }

    Vector3 operator * (Vector3 other) {
        Vector3 result = *this;
        result.values[0] *= other.values[0];
        result.values[1] *= other.values[1];
        result.values[2] *= other.values[2];
        return result;
    }

    Vector3 operator / (float other) {
        Vector3 result = *this;
        result.values[0] /= other;
        result.values[1] /= other;
        result.values[2] /= other;
        return result;
    }

    Vector3 operator + (Vector3 other) {
        Vector3 result = *this;
        result.values[0] += other.values[0];
        result.values[1] += other.values[1];
        result.values[2] += other.values[2];
        return result;
    }

    Vector3 operator - (Vector3 other) {
        Vector3 result = *this;
        result.values[0] -= other.values[0];
        result.values[1] -= other.values[1];
        result.values[2] -= other.values[2];
        return result;
    }

    Vector3 operator - (float other) {
        Vector3 result = *this;
        result.values[0] -= other;
        result.values[1] -= other;
        result.values[2] -= other;
        return result;
    }

    float operator [] (int index) {
        if (index < 0 || index > 2) {
            throw std::out_of_range("Vector3 only can be index from 0 to 2");
        }
        return values[index];
    }
};

struct Material {
    float r, g, b;
};

struct SceneObject {
    std::string type;
    Material material;
};

struct Sphere : SceneObject {
    Vector3 center;
    float radius;
};

/*
    Transforms an input from range 1 to range 2

    x Input
    in_min Minimum value or input range
    in_max Maximum value or input range
    out_min Minimum value or output range
    out_max Maximum value or output range 
*/
float map(float x, float in_min, float in_max, float out_min, float out_max) {
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
    From the camera view's orientation and position, define the viewing window.

    view_origin Defines center of the camera
    view_direction Defines direction camera is looking
    view_up Defines the up direction of the camera. Or the roll of the camera. 
    fov_h Horizontal field of view
    res_h Number of pixels in the horizontal direction
    res_w Number of pixels in the virtical direction
    background_material Color of the background pixels
*/
Mat3d create_view_window_and_ray_trace(Vector3 view_origin, Vector3 view_direction, Vector3 view_up, float fov_h, float res_h, float res_w, std::map<std::string, std::vector<SceneObject*>> scene_objects, Material background_material) 
{

    /* 
        Define the horizontal edge of the view window. Orthogonal to v and view_direction. 
        TODO:: calculation fails when view_up and view direction are co-linear. 
    */
    Vector3 u = (view_direction).cross(view_up).norm();

    /*
        Define the virtical edge of the view window. 
        Can be also defined by projecting the camera "view up" vector onto the viewing window plane.
        However, its more computationally efficient to calculate "u" first via above method, and then "v".
        TODO:: Check if the V needs to be normalized. In theory, view directin and u are already orthogonal, so v should be already unit length.
    */
    Vector3 v = u.cross(view_direction); // .norm();
    
    /*
        Find the width and the height of the viewing window in world coordinate units
    */
    float aspect_ratio = res_w / res_h;
    float w = 2.0*d* tan((0.5*fov_h) * PI / 180.0);
    float h = w / aspect_ratio;

    /* 
        Define the (x, y, z) locations of each corner of the viewing window
    */
    Vector3 n = view_direction * -1.0;
    Vector3 ul = view_origin + n*d - u*(w/2.0) + v*(h/2.0);
    Vector3 ur = view_origin + n*d + u*(w/2.0) + v*(h/2.0);
    Vector3 ll = view_origin + n*d - u*(w/2.0) - v*(h/2.0);
    // Vector3 lr = view_origin + n*d + u*(w/2.0) - v*(h/2.0);

    /*
        Now we define a mappings between pixels in the image to points in the viewing window
        Starts with the top left point, then iterates through each adding horizontal and vertical offsets
        in order to find their respective world space locations.
    */
    Vector3 default_vector3;
    std::vector<Vector3> cols(res_w, default_vector3); 
    std::vector<std::vector<Vector3>> view_window(res_h, cols);
    Vector3 delta_h = (ur - ul) / (res_w - 1.0); 
    Vector3 delta_v = (ll - ul) / (res_h - 1.0);


    /*
        For each pixel in the view port (image), define a ray from the view origin to the world location correspondind to that pixel.
        Then for each ray, cycle through scene objects. Detect which objects the ray intersects, returning the one closest to the camera.
    */
    Mat3d matt(static_cast<int>(res_h), std::vector<std::vector<byte>>(static_cast<int>(res_w), std::vector<byte>(3,0)));
    for (int i = 0; i < res_w; i++) {
        for (int j = 0; j < res_h; j++) {
            view_window[j][i] = ul + (delta_h * static_cast<float>(i)) + (delta_v * static_cast<float>(j));
            Material pixel_material = background_material;
            float distance = std::numeric_limits<float>::max();

            /*
                Form a ray pointing from view origin through a given point on the view window.
                Then, using equation of a sphere, we can find the distance to intersection on the sphere's surface.
            */

            Vector3 ray = (view_window[j][i] - view_origin).norm();
            for ( auto [type, objects] : scene_objects) {
                
                if (type == "sphere") 
                {
                    for (int k = 0; k < (int)objects.size(); k++) {
                        SceneObject* object = objects.at(k);
                        Sphere* sphere_object = (Sphere*)object;
                        Vector3 dir = sphere_object->center - view_origin;
                        
                        float A, B, C;
                        A = ray.mag();
                        B = 2.0 * (ray.dot(dir));
                        C = (dir).mag() - (sphere_object->radius * sphere_object->radius);

                        /*
                            When the sign of the determinant is Positive, there are two solutions.
                            When the sign of the determinant is Negative, there are no solutions.
                            When the determinant is Zero, there there is one solution.
                        */
                        float determinant = std::pow(B, 2.0) - (4.0 * A * C);
                        if (!std::signbit(determinant)) {
                            float t_1 = (-B + std::sqrt(determinant)) / (2.0 * A);
                            float t_2 = (-B - std::sqrt(determinant)) / (2.0 * A);

                            /*
                                Positive distance indicates object in front of "eye".
                            */
                            if (t_1 > 0.0) {
                                if (t_1 < distance) {
                                    distance = t_1;
                                    pixel_material = sphere_object->material;
                                } 
                            } 

                            if (t_2 > 0.0) {
                                if (t_2 < distance) {
                                    distance = t_2;
                                    pixel_material = sphere_object->material;
                                } 
                            }
                        } else if (determinant == 0) {
                            float t = -B / 2.0;

                            if (t > 0.0) {
                                if (t < distance) {
                                    distance = t;
                                    pixel_material = sphere_object->material;
                                }
                            }
                        }
                    }
                }
            }

            matt[j][i][0] = static_cast<byte>(map(pixel_material.r, 0, 1.0, MIN_PIXEL_VALUE, MAX_PIXEL_VALUE));
            matt[j][i][1] = static_cast<byte>(map(pixel_material.g, 0, 1.0, MIN_PIXEL_VALUE, MAX_PIXEL_VALUE));
            matt[j][i][2] = static_cast<byte>(map(pixel_material.b, 0, 1.0, MIN_PIXEL_VALUE, MAX_PIXEL_VALUE));
        }
    }  

    return matt;
}


/*
    Checks if string arguement is in valid arugments lists
    q Arguement to check
    scene_objects Whether to check the scene objects list 
*/
bool check_args(std::string q, bool scene_objects = false)
{
    if (scene_objects) {
        for(int x = 0; x < num_obj_types; x++){
            if (object_types[x].find(q, 0) != std::string::npos){
                return true;
            }
        }
    } else {
        for(int x = 0; x < num_commands; x++){
            if (valid_commands[x].find(q, 0) != std::string::npos){
                return true;
            }
        }
    }

    return false;
}


/*
    Args for this programs:

    eye   eyex eyey eyez
    viewdir   vdirx  vdiry  vdirz
    updir   upx  upy  upz
    hfov   fovh
    imsize   width  height
    bkgcolor   r  g  b
    mtlcolor   r  g  b
    sphere   cx  cy  cz  r
    projection  parallel
    cylinder   cx  cy  cz  dirx  diry  dirz  radius  length
    
*/
int main(int argc,char* argv[])
{
    std::map<std::string, std::vector<std::string>> commands;
    std::map<std::string, std::vector<SceneObject*>> scene_objects;
    
    if(argc > 1)
    {
        /*
            Find and Read in image properties file
            Then parse tokens from image properties file.
            Store commands and arguements for later use
        */
        std::string input_file_name{argv[1]};
        std::ifstream input_file(input_file_name);
        std::string image_properties_string;
        if ( input_file.is_open() ) {
            
            // Read in one line at a time
            while (std::getline(input_file, image_properties_string)){
                std::vector<std::string> image_properties;
                std::istringstream ss(image_properties_string);
                std::string del;

                // Strip line of whitespace
                while(std::getline(ss, del, ' ')) {
                    if (del != "") {
                        image_properties.push_back(del);
                    }
                }

                // If arguement is a scene properties command
                if (image_properties.size() > 1 && check_args(image_properties[0]) ) {
                    std::string command = image_properties[0];
                    image_properties.erase(image_properties.begin());
                    commands[command] = image_properties;
                
                // If arguement is a scene object
                } else if (image_properties.size() > 1 && check_args(image_properties[0], true)) {
                    if (image_properties[0] == "sphere") {
                        try
                        {
                            Sphere* new_obj = new Sphere();
                            new_obj->type = image_properties[0];
                            new_obj->radius = std::stof(image_properties[4]);
                            new_obj->center = {
                                {std::stof(image_properties[1]), std::stof(image_properties[2]), std::stof(image_properties[3])}
                            };

                            new_obj->material.r = std::stof(commands.at("mtlcolor")[0]);
                            new_obj->material.g = std::stof(commands.at("mtlcolor")[1]);
                            new_obj->material.b = std::stof(commands.at("mtlcolor")[2]);

                            scene_objects[image_properties[0]].push_back(
                                (SceneObject*) new_obj
                            );
                        }
                        catch(const std::exception& e)
                        {
                            std::cout << "ERROR: Invalid args for 'sphere' object. Please verify." << std::endl;
                            return 0;
                        }
                    }
                }
            }
        } else {
            std::cout << "ERROR: Issue reading input file '" << input_file_name << "'. " << "Please verify path." << std::endl;
            return 0;
        }
        
        /*
            Extract height and width. Validate Correctness.
        */ 
        int height, width;
        if (commands.find("imsize") == commands.end()) {
            std::cout << "Error: Requires command 'imsize'" << std::endl;
            return 0;
        } else {
            try
            {
                height = std::stoi(commands.at("imsize")[1]);
            }
            catch(const std::exception& e)
            {
                std::cout << "ERROR: Invalid image dimensions. Please verify." << std::endl;
                return 0;
            }

            try
            {
                width = std::stoi(commands.at("imsize")[0]);
            }
            catch(const std::exception& e)
            {
                std::cout << "ERROR: Invalid image dimensions. Please verify." << std::endl;
                return 0;
            } 

            if (height <= 1 || width <= 1) {
                std::cout << "ERROR: Invalid image dimensions. Please verify." << std::endl;
                return 0;
            }
        }

        /*
            Extract view origin. Validate Correctness.
        */
        Vector3 view_origin; 
        if (commands.find("eye") == commands.end()) {
            std::cout << "Error: Requires command 'eye'" << std::endl;
            return 0;
        } else {
            try
            {
                view_origin.values[0] = std::stof(commands.at("eye")[0]);
                view_origin.values[1] = std::stof(commands.at("eye")[1]);
                view_origin.values[2] = std::stof(commands.at("eye")[2]);
            }
            catch(const std::exception& e)
            {
                std::cout << "ERROR: Invalid args for 'eye' command. Please verify." << std::endl;
                return 0;
            }
        }

    
        /*
            Extract view direction. Validate Correctness.
        */
        Vector3 view_direction;
        if (commands.find("viewdir") == commands.end()) {
            std::cout << "Error: Requires command 'viewdir'" << std::endl;
            return 0;
        } else {
            try
            {
                view_direction.values[0] = std::stof(commands.at("viewdir")[0]);
                view_direction.values[1] = std::stof(commands.at("viewdir")[1]);
                view_direction.values[2] = std::stof(commands.at("viewdir")[2]);
            }
            catch(const std::exception& e)
            {
                std::cout << "ERROR: Invalid args for 'viewdir' command. Please verify." << std::endl;
                return 0;
            }
        }

        /*
            Extract view up. Validate Correctness.
        */
        Vector3 view_up;
        if (commands.find("updir") == commands.end()) {
            std::cout << "Error: Requires command 'updir'" << std::endl;
            return 0;
        } else {
            try
            {
                view_up.values[0] = std::stof(commands.at("updir")[0]);
                view_up.values[1] = std::stof(commands.at("updir")[1]);
                view_up.values[2] = std::stof(commands.at("updir")[2]);
            }
            catch(const std::exception& e)
            {
                std::cout << "ERROR: Invalid args for 'updir' command. Please verify." << std::endl;
                return 0;
            }
        }

        /*
            Extract horizontal FOV. Validate Correctness.
        */
        float fov_h;
        if (commands.find("hfov") == commands.end()) {
            std::cout << "Error: Requires command 'hfov'" << std::endl;
            return 0;
        } else {
            try
            {
                fov_h = std::stof(commands.at("hfov")[0]);
            }
            catch(const std::exception& e)
            {
                std::cout << "ERROR: Invalid args for 'hfov' command. Please verify." << std::endl;
                return 0;
            }
        }
        
        /*
            Extract backgroun material. Validate Correctness.
        */
        Material background_material;
        if (commands.find("bkgcolor") == commands.end()) {
            std::cout << "Error: Requires command 'bkgcolor'" << std::endl;
            return 0;
        } else {
            try
            {
                background_material.r = std::stof(commands.at("bkgcolor")[0]);
                background_material.g = std::stof(commands.at("bkgcolor")[1]);
                background_material.b = std::stof(commands.at("bkgcolor")[2]);
            }
            catch(const std::exception& e)
            {
                std::cout << "ERROR: Invalid args for 'bkgcolor' command. Please verify." << std::endl;
                return 0;
            }
        }

        /*
            Using previous commands, build scene viewing window and raytrace.
        */
        Mat3d matt = create_view_window_and_ray_trace(view_origin, view_direction, view_up, fov_h, height, width, scene_objects, background_material); 


        /*
            Now write the resulting image matt to ppm file
        */
        std::ofstream image_stream;
        std::string file_name = argv[1];
        remove_extension(file_name);
        std::string file_name_ppm = file_name  + ".ppm";
        image_stream.open(file_name_ppm);

        if (image_stream.fail())
        {
            std::cout << "ERROR: failed to create ppm image" << std::endl;
            return 0;
        }

        /*
            Create the image header
        */ 
        image_stream << "P3 " << std::endl;
		image_stream << width << " " << height << " " << std::endl;		
		image_stream << "255 " << std::endl;

        /*
            Now print image to file
        */
        for (int j = height - 1; j >= 0; --j) {
            for (int i = width - 1; i >= 0; --i) { 
                image_stream << std::to_string(matt[j][i][0]) << " ";
                image_stream << std::to_string(matt[j][i][1]) << " ";
                image_stream << std::to_string(matt[j][i][2]) << " " << std::endl;
            }
        }

        image_stream.close();

    } else {
        std::cout << "Error: Incorrect number of arguements in input file. Please follow this formate: imsize width height" << std::endl;
    }

    return 0;
}
