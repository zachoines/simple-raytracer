#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <numeric>
#include <algorithm>
#include <cmath> 
#include <map>

typedef unsigned char byte;

#define MIN_PIXEL_VALUE 0
#define MAX_PIXEL_VALUE 255
#define PI 3.14159265
#define d 5.0 // Can be any number
#define num_commands 7
#define num_obj_types 2
#define Mat2d std::vector<std::vector<byte>>
#define Mat3d std::vector<Mat2d>

const std::string valid_commands[] = {"eye", "viewdir", "updir", "hfov", "imsize", "bkgcolor", "mtlcolor"};
const std::string object_types[] = {"sphere", "light"}; // Object is anything with multiple instances

struct Vector3 
{
    float values[3] = { 0.0 };

    float x () 
    {
        return values[0];
    }

    float y () 
    {
        return values[1];
    }

    float z () 
    {
        return values[2];
    }

    float sum() 
    {
        return values[0] + values[1] + values[2];
    } 

    float dot(Vector3 other) 
    {
        return (*this * other).sum();
    }

    float mag() 
    {
        return std::sqrt(this->square().sum());
    }

    Vector3 square() 
    {
        Vector3 result = *this;
        result.values[0] *= result.values[0];
        result.values[1] *= result.values[1];
        result.values[2] *= result.values[2];
        return result;
    } 

    Vector3 cross (Vector3 other) 
    {
        Vector3 result;
        result.values[0]=this->y()*other.z() - this->z()*other.y();
        result.values[1]=this->z()*other.x() - this->x()*other.z();
        result.values[2]=this->x()*other.y() - this->y()*other.x();
        return result;
    }
    
    Vector3 norm() 
    {
        return *this / mag();
    }

    // Op overloads
    Vector3 operator * (float other) 
    {
        Vector3 result = *this;
        result.values[0] *= other;
        result.values[1] *= other;
        result.values[2] *= other;
        return result;
    }

    Vector3 operator * (Vector3 other) 
    {
        Vector3 result = *this;
        result.values[0] *= other.values[0];
        result.values[1] *= other.values[1];
        result.values[2] *= other.values[2];
        return result;
    }

    Vector3 operator / (float other) 
    {
        Vector3 result = *this;
        result.values[0] /= other;
        result.values[1] /= other;
        result.values[2] /= other;
        return result;
    }

    Vector3 operator / (Vector3 other) 
    {
        Vector3 result = *this;
        result.values[0] /= other.values[0];
        result.values[1] /= other.values[1];
        result.values[2] /= other.values[2];
        return result;
    }

    Vector3 operator + (Vector3 other) 
    {
        Vector3 result = *this;
        result.values[0] += other.values[0];
        result.values[1] += other.values[1];
        result.values[2] += other.values[2];
        return result;
    }

    Vector3 operator - (Vector3 other) 
    {
        Vector3 result = *this;
        result.values[0] -= other.values[0];
        result.values[1] -= other.values[1];
        result.values[2] -= other.values[2];
        return result;
    }

    Vector3 operator - (float other) 
    {
        Vector3 result = *this;
        result.values[0] -= other;
        result.values[1] -= other;
        result.values[2] -= other;
        return result;
    }

    float operator [] (int index) 
    {
        if (index < 0 || index > 2) {
            throw std::out_of_range("Vector3 only can be index from 0 to 2");
        }
        return values[index];
    }
};


struct Color 
{
    float r, g, b;
    
    Color operator * (Color other) 
    {
        Color result = *this;
        result.r = std::clamp(other.r * result.r, 0.0f, 1.0f);
        result.g = std::clamp(other.g * result.g, 0.0f, 1.0f);
        result.b = std::clamp(other.b * result.b, 0.0f, 1.0f);
        return result;
    }

    Color operator * (float other) 
    {
        Color result = *this;
        result.r = std::clamp(other * result.r, 0.0f, 1.0f);
        result.g = std::clamp(other * result.g, 0.0f, 1.0f);
        result.b = std::clamp(other * result.b, 0.0f, 1.0f);
        return result;
    }

    Color operator + (Color other) 
    {
        Color result = *this;
        result.r = std::clamp(other.r + result.r, 0.0f, 1.0f);
        result.g = std::clamp(other.g + result.g, 0.0f, 1.0f);
        result.b = std::clamp(other.b + result.b, 0.0f, 1.0f);
        return result;
    }
};

/*
    Material Defined by the Phong illumination Model:
    diffuse Diffuse color
    specular Specular color
    ka Ambient reflectivity weight
    kd Diffuse reflectivity weight
    ks Specular reflectivity weight
    n Specular highlight falloff

*/
struct Material 
{
    Color diffuse;
    Color specular;
    float ka, kd, ks, n;
};

struct SceneObject 
{
    int id;
    std::string type;
    Material material;
};

struct Sphere : SceneObject 
{
    Vector3 center;
    float radius;
};

struct Light
{
    Vector3 position; // For positional lights
    Vector3 direction; // For directional lights
    float w;
    Color color;
};

/*
    Function hoisting
*/
Color ShadeRay(SceneObject* intersected_object, Vector3 object_intersection, Vector3 view_origin, Vector3 view_direction, Material material, Vector3 world_location, Vector3 surface_normal, std::vector<Light> scene_lights, std::map<std::string, std::vector<SceneObject*>> scene_objects) ;
std::map<SceneObject*, std::vector<float>> TraceRay(std::map<std::string, std::vector<SceneObject*>> scene_objects, Vector3 view_origin, Vector3 ray);

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
    material Material of the object hit by ray 
    world_location World location of intersection point of object and ray
    surface_normal The normal vector of the object where ray hits
    scene_lights All the lights currently in the scene
*/
Color ShadeRay(SceneObject* intersected_object, Vector3 object_intersection, Vector3 view_origin, Vector3 view_direction, Material material, Vector3 surface_normal, std::vector<Light> scene_lights, std::map<std::string, std::vector<SceneObject*>> scene_objects) 
{
    Vector3 V = view_direction * -1.0;
    Vector3 N = surface_normal;
    Color ambient = material.diffuse * material.ka;
    Color tmp = { 0.0, 0.0, 0.0 };
    Color shadow_mask = { 1.0, 1.0, 1.0 };
    std::vector<bool> obstructions;
    
    for (Light light : scene_lights) 
    {
        Vector3 L, H;
        bool obstructed = false;

        /*
            If directional light source. These are at infinite distance. All rays point in same direction.
        */
        if (light.w == 0) 
        {
            L = light.direction.norm() * -1.0;


            /*
                Determine if shadow exists:
                Ray-trace along the negative of light's direction for a directional light
                For directional lights, if intersection distance is greater than 0, then a shadow will be cast.
            */
            Vector3 ray = light.direction * -1;
            std::map<SceneObject*, std::vector<float>> objects_intersections = TraceRay(scene_objects, object_intersection, ray);

            for ( auto [object, distances] : objects_intersections) 
            {    
                if (object->type == "sphere")
                {
                    /*
                        For spheres, we do not consider self intersections
                    */
                    if (object->id == intersected_object->id) 
                    {
                        continue;
                    }
                    
                    for (float distance : distances) 
                    {
                        if (distance > 0.0) 
                        {
                            shadow_mask = { 0.0, 0.0, 0.0 };
                            obstructed = true;
                        }
                    }    
                }
            }
        }

        /*
            Otherwise its a point light source, which emit light in all directions at once. Points from objects surface to light. 
        */
        else 
        {
            L = (light.position - object_intersection).norm();
            float distance_to_light = std::sqrt((object_intersection - light.position).square().sum());

            /*
                Determine if shadow exists:
                Ray-trace from point of intersection to light source, detecting other scene objects are occluding light.
            */
            Vector3 ray = (light.position - object_intersection).norm();
            std::map<SceneObject*, std::vector<float>> objects_intersections = TraceRay(scene_objects, object_intersection, ray);
          
            for ( auto [object, distances] : objects_intersections) 
            {    
                if (object->type == "sphere") 
                {
                    /*
                        For spheres, we do not consider self intersections
                    */
                    if (object->id == intersected_object->id) {
                        continue;
                    }
                    
                    /*
                        Find if object intersection is in-between object and light source
                    */ 
                    for (float distance : distances) 
                    {
                        if (distance > 0.0 && distance < distance_to_light) 
                        {
                            shadow_mask = { 0.0, 0.0, 0.0 };
                            obstructed = true;
                        }
                    }    
                }
            }

            obstructions.push_back(obstructed);
        }
        
        /*
            For multi-light setups.
            Ensures an intersection point is fully in shadow or if another light still shines on it.
        */
        // for (bool o : obstructions ) {
        //     if (o == false) {
        //         shadow_mask = { 1.0, 1.0, 1.0 };
        //     }
        // }

        /*
            Unit vector halfway between the direction to the light and the direction to the viewer 
            This is the direction associated with the brightest highlight
        */
        H = ((L + V) / 2.0).norm();
        Color diffuse = (material.diffuse * material.kd) * std::max(0.0f, N.dot(L)); 
        Color specular = (material.specular * material.ks) * pow(std::max(0.0f, N.dot(H)), material.n); 
        tmp = tmp + (light.color * shadow_mask * (diffuse + specular));
    }
    
    return ambient + tmp;
}


/*
    Returns object and their intersection points with provided ray
*/
std::map<SceneObject*, std::vector<float>> TraceRay(std::map<std::string, std::vector<SceneObject*>> scene_objects, Vector3 view_origin, Vector3 ray) 
{
    std::map<SceneObject*, std::vector<float>> objects_intersections;
    std::vector<float> distances;
    for ( auto [type, objects] : scene_objects) {
        if (type == "sphere") 
        {
            for (int k = 0; k < (int)objects.size(); k++) 
            {
                std::vector<float> distances;
                SceneObject* object = objects.at(k);
                Sphere* sphere_object = (Sphere*)object;
                Vector3 dir = (view_origin - sphere_object->center);

                /*
                    Determine ray intersection points (x, y, z) via equation of sphere:
                    (x–sphere.x)^2 + (y–sphere.y)^2 + (z–sphere.z)^2 == sum[(intersection - sphere_center)^2] == sphere.r^2
                    intersection = view_origin + (distance * ray)
                */
                float A = 1.0; // ray.square().sum();
                float B = 2.0 * (ray * dir).sum();
                float C = dir.square().sum() - pow(sphere_object->radius, 2.0);

                /*
                    When the sign of the determinant is Positive, there are two solutions.
                    When the sign of the determinant is Negative, there are no solutions.
                    When the determinant is Zero, there there is one solution.
                */
                float determinant = std::pow(B, 2.0) - (4.0 * A * C);
                if (!std::signbit(determinant)) {
                    distances.push_back((-B + std::sqrt(determinant)) / (2.0 * A));
                    distances.push_back((-B - std::sqrt(determinant)) / (2.0 * A));
                } else if (determinant == 0) {
                    distances.push_back(-B / 2.0);
                }

                objects_intersections[object] = distances;

                /* Usefull assertion */
                // sphere_object->radius == sqrtf(((sphere_intersection - sphere_object->center).square().sum()));     
            }
        }
    }
   
    return objects_intersections;
}

/*
    Define the viewing window and begin ray tracing to determine color value of each pixel:

    view_origin Defines center of the camera
    view_direction Defines direction camera is looking
    view_up Defines the up direction of the camera. Or the roll of the camera. 
    fov_h Horizontal field of view
    res_h Number of pixels in the horizontal direction
    res_w Number of pixels in the virtical direction
    background_color Color of the background pixels
*/
Mat3d create_view_window_and_ray_trace(Vector3 view_origin, Vector3 view_direction, Vector3 view_up, float fov_h, float res_h, float res_w, std::map<std::string, std::vector<SceneObject*>> scene_objects, std::vector<Light> scene_lights, Color background_color) 
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
    Vector3 v = u.cross(view_direction); //.norm();
    
    /*
        Find the width and the height of the viewing window in world coordinate units
    */
    float aspect_ratio = res_w / res_h;
    float w = 2.0*d* tan((0.5*fov_h) * PI / 180.0);
    float h = w / aspect_ratio;

    /* 
        Define the (x, y, z) locations of each corner of the viewing window
    */
    Vector3 n = view_direction;
    Vector3 ul = view_origin + n*d - u*(w/2.0) + v*(h/2.0);
    Vector3 ur = view_origin + n*d + u*(w/2.0) + v*(h/2.0);
    Vector3 ll = view_origin + n*d - u*(w/2.0) - v*(h/2.0);

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
            Color pixel_color = background_color;
            float min_distance = std::numeric_limits<float>::max();

            /*
                Form a ray pointing from view origin through a given point on the view window.
                Then, ray-trace through scene finding intersecting objects
            */
            Vector3 ray = (view_window[j][i] - view_origin).norm();
            std::map<SceneObject*, std::vector<float>> objects_intersections = TraceRay(scene_objects, view_origin, ray);

            for ( auto [object, distances] : objects_intersections) 
            {    
                if (object->type == "sphere") 
                {
                    for (float distance : distances) 
                    {
                        if (distance < min_distance) {
                            min_distance = distance;
                            Sphere* sphere_object = (Sphere*)object;
                            Vector3 sphere_intersection = view_origin + (ray * distance);               
                            Vector3 surface_normal = (sphere_intersection - sphere_object->center) / sphere_object->radius; 
                            pixel_color = ShadeRay(object, sphere_intersection, view_origin, view_direction, sphere_object->material, surface_normal, scene_lights, scene_objects);
                        }
                    }    
                }
            }

            matt[j][i][0] = static_cast<byte>(map(pixel_color.r, 0, 1.0, MIN_PIXEL_VALUE, MAX_PIXEL_VALUE));
            matt[j][i][1] = static_cast<byte>(map(pixel_color.g, 0, 1.0, MIN_PIXEL_VALUE, MAX_PIXEL_VALUE));
            matt[j][i][2] = static_cast<byte>(map(pixel_color.b, 0, 1.0, MIN_PIXEL_VALUE, MAX_PIXEL_VALUE));
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
    mtlcolor   Od Od Od Os Os Os ka kd ks n
    light x y z w r g b
    sphere   cx  cy  cz  r
*/
int main(int argc,char* argv[])
{
    std::map<std::string, std::vector<std::string>> commands;
    std::map<std::string, std::vector<SceneObject*>> scene_objects;
    std::vector<Light> scene_lights;
    int obj_id_counter = 0;
    
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
                    obj_id_counter++;
                    if (image_properties[0] == "sphere") 
                    {
                        Sphere* new_obj = new Sphere();
                        try
                        {
                            new_obj->id = obj_id_counter;
                            new_obj->type = image_properties[0];
                            new_obj->radius = std::stof(image_properties[4]);
                            new_obj->center = {
                                {std::stof(image_properties[1]), std::stof(image_properties[2]), std::stof(image_properties[3])}
                            };
                        }
                        catch(const std::exception& e)
                        {
                            std::cout << "ERROR: Invalid args for 'sphere' object. Please verify." << std::endl;
                            return 0;
                        }

                        try
                        {
                            new_obj->material.diffuse.r = std::stof(commands.at("mtlcolor")[0]);
                            new_obj->material.diffuse.g = std::stof(commands.at("mtlcolor")[1]);
                            new_obj->material.diffuse.b = std::stof(commands.at("mtlcolor")[2]);
                            new_obj->material.specular.r = std::stof(commands.at("mtlcolor")[3]);
                            new_obj->material.specular.g = std::stof(commands.at("mtlcolor")[4]);
                            new_obj->material.specular.b = std::stof(commands.at("mtlcolor")[5]);
                            new_obj->material.ka = std::stof(commands.at("mtlcolor")[6]);
                            new_obj->material.kd = std::stof(commands.at("mtlcolor")[7]);
                            new_obj->material.ks = std::stof(commands.at("mtlcolor")[8]);
                            new_obj->material.n = std::stof(commands.at("mtlcolor")[9]);
                            
                        }
                        catch(const std::exception& e)
                        {
                            std::cout << "ERROR: Invalid args for 'mtlcolor' command. Please verify." << std::endl;
                            return 0;
                        }

                        scene_objects[image_properties[0]].push_back(
                            (SceneObject*) new_obj
                        );
                    } 
                    else if (image_properties[0] == "light") 
                    {
                        /*
                            Extract light. Validate Correctness.
                        */
                        Light light;
                        try {
                            light.w = std::stof(image_properties[4]);
                            if (light.w == 0) {
                                light.direction.values[0] = std::stof(image_properties[1]);
                                light.direction.values[1] = std::stof(image_properties[2]);
                                light.direction.values[2] = std::stof(image_properties[3]); 
                            } else {
                                light.position.values[0] = std::stof(image_properties[1]);
                                light.position.values[1] = std::stof(image_properties[2]);
                                light.position.values[2] = std::stof(image_properties[3]); 
                            }

                            light.color.r = std::stof(image_properties[5]);
                            light.color.g = std::stof(image_properties[6]);
                            light.color.b = std::stof(image_properties[7]);
                        }
                        catch(const std::exception& e)
                        {
                            std::cout << "ERROR: Invalid args for 'light' command. Please verify." << std::endl;
                            return 0;
                        }
                        
                        scene_lights.push_back(light);
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
            Extract background color. Validate Correctness.
        */
        Color background_color;
        if (commands.find("bkgcolor") == commands.end()) {
            std::cout << "Error: Requires command 'bkgcolor'" << std::endl;
            return 0;
        } else {
            try
            {
                background_color.r = std::stof(commands.at("bkgcolor")[0]);
                background_color.g = std::stof(commands.at("bkgcolor")[1]);
                background_color.b = std::stof(commands.at("bkgcolor")[2]);
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
        Mat3d matt = create_view_window_and_ray_trace(view_origin, view_direction, view_up, fov_h, height, width, scene_objects, scene_lights, background_color); 


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
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
             
                image_stream << std::to_string(matt[i][j][0]) << " ";
                image_stream << std::to_string(matt[i][j][1]) << " ";
                image_stream << std::to_string(matt[i][j][2]) << " " << std::endl;
            }
        }

        image_stream.close();

    } else {
        std::cout << "Error: Incorrect number of arguements in input file. Please follow this formate: imsize width height" << std::endl;
    }

    return 0;
}
