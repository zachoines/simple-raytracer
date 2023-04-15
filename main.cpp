#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include "src/definitions.h"
#include "src/utility.h"

/*
    Function hoisting
*/
Mat3D create_view_window_and_ray_trace(
    Vector3 view_origin, 
    Vector3 view_direction, 
    Vector3 view_up, 
    float fov_h, 
    float res_h, 
    float res_w, 
    Color background_color
);
std::vector<ObjectIntersections> TraceRay(
    Vector3 view_origin, 
    Vector3 ray
);
Color ShadeRay(
    Vector3 incidence_ray, 
    SceneObjectInfo* incidence_object_info, 
    Intersection incidence_object_intersection, 
    float incidence_refraction_index, 
    float transmission_refraction_index, 
    std::vector<SceneObjectInfo*> incident_object_stack,
    RayState ray_state,
    float recursion_depth,
    Color background_color
);

/*
    Valid Args for config file:

    eye eyex eyey eyez                         (The location of the 'eye' within scene)
    viewdir vdirx  vdiry  vdirz                (Defines the direction the 'eye' is looking)
    updir upx  upy  upz                        (Roll of the camera)
    hfov fovh                                  (Horizonal field of view)
    imsize width  height                       (Output image dimentions)
    bkgcolor r  g  b  η                        (Scene background color. Param η is optional) 
    mtlcolor Od Od Od Os Os Os ka kd ks n α η  (Material color. Params α η are optional)
    texture texture.ppm                        (Texture to apply to model)           
    light x y z w r g b                        (Scene light. Directional or point)
    sphere cx  cy  cz  r                       (Sphere defined by center and radiusm)
    vn nx ny nz                                (Vertex normal)
    vt tx ty                                   (texture coordinates)
    #                                          (Single line comment)
    f v1/vt1/vn1 v2/vt2/vn2 v3/vt2/vn          (smooth-shaded, textured triangle)
    f v1//vn1 v2//vn2 v3//vn3                  (smooth-shaded, untextured triangle)
    f v1/vt1 v2/vt2 v3/vt2                     (non-smooth-shaded, textured triangle)
*/

Globals environment; 

int main(int argc,char* argv[])
{
    // Storage
    std::vector<Texture*> textures;
    std::map<unsigned int, Point> texture_coords;
    std::map<unsigned int, Vector3> vertices;
    std::map<unsigned int, Vector3> normals;

    // Counters for unique object id's
    unsigned int obj_id_counter = 0;
    unsigned int vertex_counter = 0;
    unsigned int normal_counter = 0;
    unsigned int texture_coord_counter = 0;

    // Will toggle between these two when reading in commands
    Texture* current_texture = nullptr;
    Material current_material;
    bool use_texture = false;
    bool has_material = false;

    // Scene related variables
    Vector3 view_origin;
    int height, width;
    Vector3 view_direction;
    Vector3 view_up;
    float fov_h;
    Color background_color;
    
    if(argc > 1)
    {
        /*
            Find and Read in image properties file
            Then parse tokens from image properties file.
            Store commands and arguments for later use
        */
        std::string input_file_name{argv[1]};
        std::ifstream input_file(input_file_name);
        std::string image_properties_string;

        // Put environment variables
        environment.other["recursion_depth"] = 4.0;
        environment.other["epsilon"] = 1.0e-3;

        if ( input_file.is_open() ) {
            
            // Read in one line at a time
            while (std::getline(input_file, image_properties_string)){
                std::vector<std::string> arguments;
                std::istringstream ss(image_properties_string);
                std::string del;

                // Strip line of whitespace
                while(std::getline(ss, del, ' ')) {
                    // If not blank string or comment
                    if (!del.empty() || del.at(0) != '#') {
                        arguments.push_back(del);
                    }
                }

                if (arguments.size() == 0) {
                    continue;
                }

                std::string command = arguments[0];
                arguments.erase(arguments.begin());

                // Hoist switch variables here
                Texture* new_texture = nullptr;
                SceneObjectInfo* sphere_object_info = nullptr;
                Sphere* sphere_object = nullptr;
                Face* triangle = nullptr;
                SceneObjectInfo* face_object_info = nullptr;
                Light light;
                Material material;

                // If blank line or invalid command
                if (arguments.size() == 0 || argsStringValues.find(command) == argsStringValues.end()) {
                    continue;
                } else {
                    try
                    {
                        switch (argsStringValues[command])
                        {
                        case ArgValues::eye:
                            /* 
                                Extract view origin. Validate correctness.
                            */
                            environment.commands[command] = arguments;
                            
                            try
                            {
                                view_origin = {
                                    .x = std::stof(arguments[0]),
                                    .y = std::stof(arguments[1]),
                                    .z = std::stof(arguments[2])
                                };
                            }
                            catch(const std::exception& e)
                            {
                                std::cerr << e.what() << std::endl;
                                throw std::invalid_argument("ERROR: Invalid args for 'eye' command. Please verify.");
                            }
                            break;
                        case ArgValues::viewdir:
                            /*
                                Extract view direction. Validate Correctness.
                            */
                            environment.commands[command] = arguments;
                            try
                            {
                                view_direction = {
                                    .x = std::stof(arguments[0]),
                                    .y = std::stof(arguments[1]),
                                    .z = std::stof(arguments[2])
                                };
                            }
                            catch(const std::exception& e)
                            {
                                std::cerr << e.what() << std::endl;
                                throw std::invalid_argument("ERROR: Invalid args for 'viewdir' command. Please verify.");
                            }
                            break;
                        case ArgValues::updir:
                            /*
                                Extract view up. Validate Correctness.
                            */
                            environment.commands[command] = arguments;
                            try
                            {
                                view_up = {
                                    .x = std::stof(arguments[0]),
                                    .y = std::stof(arguments[1]),
                                    .z = std::stof(arguments[2])
                                };
                            }
                            catch(const std::exception& e)
                            {
                                std::cerr << e.what() << std::endl;
                                throw std::invalid_argument("ERROR: Invalid args for 'updir' command. Please verify.");
                            }
                            break;
                        case ArgValues::hfov:
                            /*
                                Extract horizontal FOV. Validate Correctness.
                            */
                            environment.commands[command] = arguments;
                            try
                            {
                                fov_h = std::stof(arguments[0]);
                            }
                            catch(const std::exception& e)
                            {
                                std::cerr << e.what() << std::endl;
                                throw std::invalid_argument("ERROR: Invalid args for 'hfov' command. Please verify.");
                            }
                            break;
                        case ArgValues::imsize:
                            /*
                                Extract height and width. Validate Correctness.
                            */ 
                            environment.commands[command] = arguments;
                            
                            try
                            {
                                height = std::stoi(arguments[1]);
                            }
                            catch(const std::exception& e)
                            {
                                std::cerr << e.what() << std::endl;
                                throw std::invalid_argument("ERROR: Invalid image dimensions. Please verify.");
                            }

                            try
                            {
                                width = std::stoi(arguments[0]);
                            }
                            catch(const std::exception& e)
                            {
                                std::cerr << e.what() << std::endl;
                                throw std::invalid_argument("ERROR: Invalid image dimensions. Please verify.");
                            } 

                            if (height <= 1 || width <= 1) {
                                throw std::invalid_argument("ERROR: Invalid image dimensions. Please verify.");
                            }
                            
                            break;
                        case ArgValues::bkgcolor:
                            /*
                                Extract background color. Validate Correctness.
                            */
                            environment.commands[command] = arguments;
                            try
                            {
                                background_color = {
                                    .r = std::stof(arguments[0]),
                                    .g = std::stof(arguments[1]),
                                    .b = std::stof(arguments[2])
                                };

                                float background_refraction_index = 0;
                                if (arguments.size() > 3) {
                                    background_refraction_index = std::stof(arguments[3]);
                                    environment.other["bkg_refraction_index"] = background_refraction_index;
                                }
                            }
                            catch(const std::exception& e)
                            {
                                std::cerr << e.what() << std::endl;
                                throw std::invalid_argument("ERROR: Invalid args for 'bkgcolor' command. Please verify.");
                            }
                            break;
                        case ArgValues::mtlcolor:
                            use_texture = false;
                            try
                            {
                                material.diffuse = {
                                    .r = std::stof(arguments[0]),
                                    .g = std::stof(arguments[1]),
                                    .b = std::stof(arguments[2])
                                };

                                material.specular = { 
                                    .r = std::stof(arguments[3]),
                                    .g = std::stof(arguments[4]),
                                    .b = std::stof(arguments[5])
                                };

                                material.ka = std::stof(arguments[6]);
                                material.kd = std::stof(arguments[7]);
                                material.ks = std::stof(arguments[8]);
                                material.n = std::stof(arguments[9]);

                                if (arguments.size() == 12) {
                                    material.opacity = std::clamp<float>(std::stof(arguments[10]), 0.0, 1.0);
                                    material.refraction_index = std::stof(arguments[11]);
                                } else {
                                    material.opacity = 1.0; // Fully opaque by default
                                    material.refraction_index = 1.0;
                                }
                                current_material = material;
                                has_material = true;
                            }
                            catch(const std::exception& e)
                            {
                                std::cerr << e.what() << std::endl;
                                throw std::runtime_error("ERROR: Issue parsing 'material' from arguments. Please verify.");
                            }
                            break;
                        case ArgValues::texture:
                            /*
                                Because an object's color/texture is relative to previous command
                                'texture' will overwrite 'mtcolor', and vice versa.
                            */
                            
                            use_texture = true;
                            try
                            {
                                new_texture = read_texture(arguments[0], new_texture);
                                current_texture = new_texture;
                                textures.push_back(new_texture);
                            }
                            catch(const std::exception& e)
                            {
                                std::cerr << e.what() << std::endl;
                                throw std::runtime_error("ERROR: Issue reading 'texture' from ppm. Please verify.");
                            }
                            break;
                        case ArgValues::sphere:
                            obj_id_counter++;
                            sphere_object = new Sphere(); 
                            sphere_object_info = new SceneObjectInfo();
                            try
                            {
                                sphere_object_info->id = obj_id_counter;
                                sphere_object_info->type = "sphere";
                                sphere_object->radius = std::stof(arguments[3]);
                                sphere_object->center = {
                                    .x = std::stof(arguments[0]), 
                                    .y = std::stof(arguments[1]), 
                                    .z = std::stof(arguments[2])
                                };
                            }
                            catch(const std::exception& e)
                            {
                                std::cerr << e.what() << std::endl;
                                throw std::invalid_argument("ERROR: Invalid args for 'sphere' object. Please verify.");
                            }

                            try
                            {
                                sphere_object_info->material = current_material;

                                if (use_texture) {
                                    if (!has_material || current_texture == nullptr) {
                                        throw std::invalid_argument("ERROR: Must define a 'mtlcolor' and 'texture'. Please verify.");
                                    }
                                    sphere_object_info->texture = current_texture;
                                    sphere_object_info->has_texture = true;
                                } else {
                                    if (!has_material) {
                                        throw std::invalid_argument("ERROR: Must define a 'mtlcolor'. Please verify.");
                                    }
                                    sphere_object_info->has_texture = false;
                                }    
                            }
                            catch(const std::exception& e)
                            {
                                std::cerr << e.what() << std::endl;
                                throw std::invalid_argument("ERROR: Invalid args for 'mtlcolor' command. Please verify.");
                            }
                            
                            sphere_object->object_info = sphere_object_info;
                            environment.scene_object_infos["sphere"].push_back(
                                sphere_object_info
                            );
                            environment.spheres[sphere_object_info->id] = sphere_object;
                            break;
                        case ArgValues::light:
                            /*
                                Extract light. Validate Correctness.
                            */
                            try {
                                light.w = std::stof(arguments[3]);
                                if (light.w == 0) {
                                    light.direction = {
                                        .x = std::stof(arguments[0]),
                                        .y = std::stof(arguments[1]),
                                        .z = std::stof(arguments[2])
                                    };
                                } else {
                                    light.position = {
                                        .x = std::stof(arguments[0]),
                                        .y = std::stof(arguments[1]),
                                        .z = std::stof(arguments[2])
                                    };
                                }

                                light.color = { 
                                    .r = std::stof(arguments[4]),
                                    .g = std::stof(arguments[5]),
                                    .b = std::stof(arguments[6])
                                };
                            }
                            catch(const std::exception& e)
                            {
                                std::cerr << e.what() << std::endl;
                                throw std::invalid_argument("ERROR: Invalid args for 'light' command. Please verify.");
                            }
                            
                            environment.scene_lights.push_back(light);
                            break;
                        case ArgValues::v:
                            /*
                                Extract Vertex. Validate Correctness.
                            */
                            try
                            {
                                vertex_counter++;
                                vertices[vertex_counter] = {
                                    .x = std::stof(arguments[0]),
                                    .y = std::stof(arguments[1]),
                                    .z = std::stof(arguments[2])
                                };
                            }
                            catch(const std::exception& e)
                            {
                                std::cerr << e.what() << std::endl;
                                throw std::invalid_argument("ERROR: Invalid args for vertex. Please verify.");
                            }
                            break;
                        case ArgValues::vn:
                            /*
                                Extract Vertex normal. Validate Correctness.
                            */
                            try
                            {
                                normal_counter++;
                                normals[normal_counter] = {
                                    .x = std::stof(arguments[0]),
                                    .y = std::stof(arguments[1]),
                                    .z = std::stof(arguments[2])
                                };
                            }
                            catch(const std::exception& e)
                            {
                                std::cerr << e.what() << std::endl;
                                throw std::invalid_argument("ERROR: Invalid args for vertex normal. Please verify.");
                            }
                            break;
                        case ArgValues::vt:
                            /*
                                Extract texture coordinate. Validate Correctness.
                            */
                            try
                            {
                                Point coord = {
                                    .x = std::stof(arguments[0]),
                                    .y = std::stof(arguments[1])
                                };

                                texture_coord_counter++;
                                texture_coords[texture_coord_counter] = coord;
                            }
                            catch(const std::exception& e)
                            {
                                std::cerr << e.what() << std::endl;
                                throw std::invalid_argument("ERROR: Invalid args for texture coordinate. Please verify.");
                            }
                            break;
                        case ArgValues::f:
                            /*
                                Extract Face. Validate Correctness.
                            */
                            triangle = new Face();
                            face_object_info = new SceneObjectInfo();
                            obj_id_counter++;
                            face_object_info->type = "face";
                            face_object_info->id = obj_id_counter; 
                            
                            try
                            {
                                for (int i = 0; i < 3; i++) {
                                    // Parse for vertex normals and/or texture coordinates
                                    unsigned int t; // Texture coord
                                    unsigned int n; // Normal
                                    unsigned int v; // Vertex
                                    if (sscanf(arguments[i].c_str(), "%d/%d/%d", &v, &t, &n ) == 3) {
                                        // success reading a face in v/t/n format. For a smooth shaded, textured triangle.
                                        triangle->vertex[i] = vertices[v];
                                        triangle->vertex_normal[i] = normals[n];
                                        triangle->smooth_shading = true;
                                        triangle->texture_coords[i] = texture_coords[t];
                                        face_object_info->has_texture = true;

                                    } else if (sscanf(arguments[i].c_str(), "%d//%d", &v, &n ) == 2) {
                                        //success reading a face in v//n format. For a smooth shaded, untextured triangle.
                                        triangle->vertex[i] = vertices[v];
                                        triangle->vertex_normal[i] = normals[n];
                                        triangle->smooth_shading = true;
                                        face_object_info->has_texture = false;

                                    } else if (sscanf(arguments[i].c_str(), "%d/%d", &v, &t) == 2) {
                                        // success reading a face in v/t format. For a non-smooth shaded, textured triangle.
                                        triangle->vertex[i] = vertices[v];
                                        triangle->smooth_shading = false;
                                        triangle->texture_coords[i] = texture_coords[t];
                                        face_object_info->has_texture = true;
                                    
                                    } else if (sscanf(arguments[i].c_str(), "%d", &v) == 1) {
                                        // success reading a face in v format; proceed accordingly
                                        triangle->vertex[i] = vertices[v];
                                        triangle->smooth_shading = false;
                                        face_object_info->has_texture = false;                               
                                    } else {
                                        // error reading face data
                                        throw std::invalid_argument("ERROR: Invalid args for 'f' object. Please verify.");
                                    }
                                };

                                // Enter material/ texture information information
                                face_object_info->material = current_material;

                                if (use_texture) {
                                    if (!has_material || current_texture == nullptr) {
                                        throw std::invalid_argument("ERROR: Must define a 'mtlcolor' and 'texture'. Please verify.");
                                    }
                                    face_object_info->texture = current_texture;
                                    face_object_info->has_texture = true;
                                } else {
                                    if (!has_material) {
                                        throw std::invalid_argument("ERROR: Must define a 'mtlcolor'. Please verify.");
                                    }
                                    face_object_info->has_texture = false;
                                }

                                // Calculate main surface normal
                                Vector3 e1 = triangle->vertex[1] - triangle->vertex[0];
                                Vector3 e2 = triangle->vertex[2] - triangle->vertex[0];
                                triangle->surface_normal = e1.cross(e2).norm();
                                triangle->object_info = face_object_info;
                                environment.faces[face_object_info->id] = triangle; 
                                environment.scene_object_infos["face"].push_back(
                                    face_object_info
                                );
                            }
                            catch(const std::exception& e)
                            {
                                std::cerr << e.what() << std::endl;
                                throw std::invalid_argument("ERROR: Invalid args for 'f' (face) object. Please verify.");
                            }
                            /* code */
                            break;
                        default:
                            continue;
                            break;
                        }
                    }
                    catch(const std::exception& e)
                    {
                        std::cerr << e.what() << std::endl;
                        throw std::invalid_argument("ERROR: Command '" + command + "' is undefined. Please verify input.");
                    }   
                }
                
            }
        } else {
            std::cout << "ERROR: Issue reading input file '" << input_file_name << "'. " << "Please verify path." << std::endl;
            return 0;
        }
        
        /*
            Assert commands have been passed 
        */
        if (environment.commands.find("imsize") == environment.commands.end()) {
            std::cout << "Error: Requires command 'imsize'" << std::endl;
            return 0;
        }
        
        if (environment.commands.find("eye") == environment.commands.end()) {
            std::cout << "Error: Requires command 'eye'" << std::endl;
            return 0;
        }
    
        if (environment.commands.find("viewdir") == environment.commands.end()) {
            std::cout << "Error: Requires command 'viewdir'" << std::endl;
            return 0;
        }
        
        if (environment.commands.find("updir") == environment.commands.end()) {
            std::cout << "Error: Requires command 'updir'" << std::endl;
            return 0;
        }
        
        if (environment.commands.find("hfov") == environment.commands.end()) {
            std::cout << "Error: Requires command 'hfov'" << std::endl;
            return 0;
        }
        
        if (environment.commands.find("bkgcolor") == environment.commands.end()) {
            std::cout << "Error: Requires command 'bkgcolor'" << std::endl;
            return 0;
        }

        /*
            Using previous commands, build scene viewing window and raytrace.
        */
        Mat3D matt = create_view_window_and_ray_trace(view_origin, view_direction.norm(), view_up.norm(), fov_h, height, width, background_color); 


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
                try
                {
                    image_stream << std::to_string(matt(i, j, 0)) << " ";
                    image_stream << std::to_string(matt(i, j, 1)) << " ";
                    image_stream << std::to_string(matt(i, j, 2)) << " " << std::endl;
                }
                catch(const std::exception& e)
                {
                    std::cerr << e.what() << std::endl;
                }  
            }
        }

        image_stream.close();

    } else {
        std::cout << "Error: Incorrect number of arguments in input file. Please follow this formate: imsize width height" << std::endl;
    }

    return 0;
}

/**
 * @brief  Define the viewing window and begin ray tracing to determine color value of each pixel
 * @returns Returns an image
 * @param view_origin The position of the camera
 * @param view_direction The forward direction the camera
 * @param view_up The up direction of camera. Determines tilt and roll.
 * @param fov_h Horizontal feild of view
 * @param res_h Height of view window
 * @param res_w Width of view window
 * @param background_color Default base color used when no ray intersections are found
**/
Mat3D create_view_window_and_ray_trace(Vector3 view_origin, Vector3 view_direction, Vector3 view_up, float fov_h, float res_h, float res_w, Color background_color) 
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
    */
    Vector3 v = u.cross(view_direction);
    
    /*
        Find the width and the height of the viewing window in world coordinate units
    */
    float aspect_ratio = res_w / res_h;
    float w = 2.0f*d* tan((0.5*fov_h) * M_PI / 180.0f);
    float h = w / aspect_ratio;

    /* 
        Define the (x, y, z) locations of each corner of the viewing window
    */
    Vector3 n = view_direction;
    Vector3 ul = view_origin + n*d - u*(w/2.0f) + v*(h/2.0f);
    Vector3 ur = view_origin + n*d + u*(w/2.0f) + v*(h/2.0f);
    Vector3 ll = view_origin + n*d - u*(w/2.0f) - v*(h/2.0f);

    /*
        Now we define a mappings between pixels in the image to points in the viewing window
        Starts with the top left point, then iterates through each adding horizontal and vertical offsets
        in order to find their respective world space locations.
    */
    Vector3 default_vector3;
    std::vector<Vector3> cols(res_w, default_vector3); 
    std::vector<std::vector<Vector3>> view_window(res_h, cols);
    Vector3 delta_h = (ur - ul) / (res_w - 1.0f); 
    Vector3 delta_v = (ll - ul) / (res_h - 1.0f);


    /*
        For each pixel in the view port (image), define a ray from the view origin to the world location correspondind to that pixel.
        Then for each ray, cycle through scene objects. Detect which objects the ray intersects, returning the one closest to the camera.
    */
    Mat3D matt(res_h, res_w, 3, 0);
    for (int i = 0; i < res_h; i++) {
        for (int j = 0; j < res_w; j++) {
            view_window[i][j] = ul + (delta_h * static_cast<float>(j)) + (delta_v * static_cast<float>(i));
            Color pixel_color = background_color;
            float min_distance = std::numeric_limits<float>::max();

            /*
                Form a ray pointing from view origin through a given point on the view window.
                Then, ray-trace through scene finding intersecting objects
            */
            Vector3 ray = (view_window[i][j] - view_origin).norm();
            std::vector<ObjectIntersections> ray_trace_results = TraceRay(view_origin, ray);
            Intersection min_intersection;
            SceneObjectInfo* intersected_object = nullptr; 
            for (auto & object_intersections : ray_trace_results) 
            {    
                for (auto & intersection : object_intersections.intersections) 
                {   
                    if (intersection.distance > 0.0f && intersection.distance < min_distance) {
                        min_distance = intersection.distance;
                        intersected_object = object_intersections.object_info;
                        min_intersection = intersection;
                    }
                }
            }

            
            if (intersected_object != nullptr) {
                std::vector<SceneObjectInfo*> incident_object_stack = { intersected_object };
                pixel_color = ShadeRay(
                    ray, 
                    intersected_object, 
                    min_intersection, 
                    environment.other["bkg_refraction_index"],
                    intersected_object->material.refraction_index,
                    incident_object_stack,
                    RayState::ENTERING,
                    environment.other["recursion_depth"],
                    background_color    
                );    
            }
            
            matt(i, j, 0) = static_cast<int>(map(pixel_color.r, 0, 1.0, MIN_PIXEL_VALUE, MAX_PIXEL_VALUE));
            matt(i, j, 1) = static_cast<int>(map(pixel_color.g, 0, 1.0, MIN_PIXEL_VALUE, MAX_PIXEL_VALUE));
            matt(i, j, 2) = static_cast<int>(map(pixel_color.b, 0, 1.0, MIN_PIXEL_VALUE, MAX_PIXEL_VALUE));
        }
    }  

    return matt;
}

/**
 * @brief Determines pixel intensity returned by a ray and object it intersects. 
 * Calulates contribution of shadows, transparency, reflections, specular/diffuse color, and so on
 * to said pixel intesity. 
 * @returns A RGB color with values in range of 0 to 1. 
 * @param incidence_ray Incoming ray  
 * @param incidence_object_info Information about object intersected by incident ray
 * @param incidence_object_intersection The point of intersection between ray and object
 * @param recursion_depth How deep we want to raytrace within scene
 * @param incidence_refraction_index Reflectivity of incident object
 * @param transmission_refraction_index Reflectivity of transmission object
 * @param ray_state Default is "ENTERING." Used to track where ray is in relation to object material during recursive calls to ShadeRay
 * @param background_color Base color utilized if no intersections are found during raytracing 
**/
Color ShadeRay(Vector3 incidence_ray, SceneObjectInfo* incidence_object_info, Intersection incidence_object_intersection, float incidence_refraction_index, float transmission_refraction_index, std::vector<SceneObjectInfo*> incident_object_stack, RayState ray_state, float recursion_depth, Color background_color)
{
    Vector3 N = incidence_object_intersection.normal;
    Vector3 I = (incidence_ray * -1.0);
    Color tmp_specular = { 0.0, 0.0, 0.0 };
    Color shadow_mask = { 1.0, 1.0, 1.0 };
    Color diffuse;
    Color tmp_reflection = { 0.0, 0.0, 0.0 };
    Color tmp_transparency = { 0.0, 0.0, 0.0 };
    Material material = incidence_object_info->material;
    std::vector<bool> obstructions;
    float cos_angle_incidence = N.dot(I);
    RayState previous_ray_state = ray_state;
    
    /*
        At point of intersection, either retreive the base diffuse color or the corresponding texture value
    */
    if (incidence_object_info->has_texture) 
    {
        if (incidence_object_info->type == "sphere") 
        {
            // Find texture coorinates
            float v = acos(N.z) / M_PI;
            float phi = atan2(N.y, N.x);
            float u;
            u = map(phi, -M_PI, M_PI, 0.0, 1.0);
            
            // Find texture pixel value that location
            float width = static_cast<float>(incidence_object_info->texture->width);
            float height = static_cast<float>(incidence_object_info->texture->height);
            v = std::clamp<float>(v, 0.0, 1.0);
            u = std::clamp<float>(u, 0.0, 1.0);

            // TODO: Map using bi-linear interpolation 
            int i = static_cast<int>(std::clamp<float>(round((height - 1.0) * v), 0.0, height - 1.0));
            int j = static_cast<int>(std::clamp<float>(round((width - 1.0) * u), 0.0, width - 1.0));
            Mat3D* image = incidence_object_info->texture->image;
            
            // Update diffuse color 
            diffuse = {
                .r = static_cast<float>(map(image->get(j, i, 0), MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, 0.0, 1.0)),
                .g = static_cast<float>(map(image->get(j, i, 1), MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, 0.0, 1.0)),
                .b = static_cast<float>(map(image->get(j, i, 2), MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, 0.0, 1.0))
            };
        } else if (incidence_object_info->type == "face") {
            Face* face = environment.faces[incidence_object_info->id];

            /* 
                Get new texture coordinate as linear combination of the 3 texture coordinates,
                using face' barycentric coordinates as weights
            */
            float u = 
                (face->barycentric_cords.x * std::clamp<float>(face->texture_coords[0].x, 0.0, 1.0)) +
                (face->barycentric_cords.y * std::clamp<float>(face->texture_coords[1].x, 0.0, 1.0)) +
                (face->barycentric_cords.z * std::clamp<float>(face->texture_coords[2].x, 0.0, 1.0));
            float v = 
                (face->barycentric_cords.x * std::clamp<float>(face->texture_coords[0].y, 0.0, 1.0)) +
                (face->barycentric_cords.y * std::clamp<float>(face->texture_coords[1].y, 0.0, 1.0)) +
                (face->barycentric_cords.z * std::clamp<float>(face->texture_coords[2].y, 0.0, 1.0));


            v = std::clamp<float>(v, 0.0, 1.0);
            u = std::clamp<float>(u, 0.0, 1.0);
        
            // Find texture pixel value that location
            float width = static_cast<float>(incidence_object_info->texture->width);
            float height = static_cast<float>(incidence_object_info->texture->height);
            
            // TODO: Map using bi-linear interpolation 
            int i = static_cast<int>(std::clamp<float>(round((width - 1.0f) * u), 0.0, width - 1.0));
            int j = static_cast<int>(std::clamp<float>(round((height - 1.0f) * v), 0.0, height - 1.0));
            Mat3D* image = incidence_object_info->texture->image;
            
            // Update diffuse color 
            diffuse = {
                .r = static_cast<float>(map(image->get(i, j, 0), MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, 0.0, 1.0)),
                .g = static_cast<float>(map(image->get(i, j, 1), MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, 0.0, 1.0)),
                .b = static_cast<float>(map(image->get(i, j, 2), MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, 0.0, 1.0)),
            };
        }
    } 
    else 
    {
        diffuse = material.diffuse;
    }

    if (cos_angle_incidence < 0.0 && incidence_object_info->type == "sphere") {
        N = (N * -1.0);
        cos_angle_incidence = N.dot(I);
    }

    /*
        Simulate shadows by tracing up from intersection point to light source.
        If the object has transparency, then the object's opacity discounts the intensity of the shadow.
    */
    for (Light light : environment.scene_lights) 
    {
        Vector3 L, H;

        /*
            If directional light source. These are at infinite distance. All rays point in same direction.
        */
        if (light.w == 0) 
        {
            L = light.direction.norm() * -1.0f;


            /*
                Determine if shadow exists:
                Ray-trace along the negative of light's direction for a directional light
                For directional lights, if intersection distance is greater than 0, then a shadow will be cast.
            */
            Vector3 ray = light.direction * -1.0f;
            std::vector<ObjectIntersections> other_objects_intersections = TraceRay(incidence_object_intersection.point, ray);

            for ( auto [object, intersections] : other_objects_intersections) 
            {    
                if (object->id == incidence_object_info->id) 
                {
                    continue;
                }
                
                for (auto intersection : intersections) 
                {
                    if (intersection.distance > environment.other["epsilon"]) 
                    {
                        shadow_mask = shadow_mask * (1.0 - object->material.opacity);
                    }
                }
            }
        }

        /*
            Otherwise its a point light source, which emit light in all directions at once. Points from objects surface to light. 
        */
        else 
        {
 
            L = (light.position - incidence_object_intersection.point).norm();
            float distance_to_light = std::sqrt((incidence_object_intersection.point - light.position).square().sum());

            /*
                Determine if shadow exists:
                Ray-trace from point of intersection to light source, detecting other scene objects are occluding light.
            */
            std::vector<ObjectIntersections> other_object_intersections = TraceRay(incidence_object_intersection.point, L);
          
            for ( auto [object, intersections] : other_object_intersections) 
            {    
                /*
                    We do not consider self intersections
                */
                if (object->id == incidence_object_info->id) {
                    continue;
                }
                
                /*
                    Find if object intersection is in-between object and light source
                */ 
                for (auto intersection : intersections) 
                {
                    if (intersection.distance > environment.other["epsilon"] && intersection.distance < distance_to_light) 
                    {
                        shadow_mask = shadow_mask * (1.0 - object->material.opacity);
                    }
                }  
            }
        }
  
        H = (L + I).norm(); // Halfway vector
        Color diffuse_component = (diffuse * material.kd) * std::max(0.0f, N.dot(L));
        Color specular_component = (material.specular * material.ks) * powf(std::max(0.0f, N.dot(H)), material.n);
        tmp_specular = tmp_specular + (light.color * shadow_mask * (
            diffuse_component + 
            specular_component
        ));
    }

    float snells_ratio = (incidence_refraction_index / transmission_refraction_index);
    float critical_angle = asinf(transmission_refraction_index / incidence_refraction_index); 
    float incidence_angle = acosf(cos_angle_incidence);
    bool total_internal_reflection = (critical_angle < incidence_angle) && (incidence_angle < (90.0 * M_PI / 180.0));
    float F_0 = powf((transmission_refraction_index - incidence_refraction_index)/(transmission_refraction_index + incidence_refraction_index), 2.0); 
    float F = F_0 + (1.0 - F_0)*powf(1.0 - (cos_angle_incidence), 5.0);
    
    /*
        Determine contribution of pixel intensity from transparency effects:
        Do this by recursively raytracing from the transmission ray "T" (Ray that enters new medium).

        Assuming the material the ray enters is not fully opaque
        or refracted back into the same medium it arrived from (total internal reflection), 
        we then trace the ray through the scene (up to "recursion_depth" times).
    */
    if (recursion_depth > 0 && !total_internal_reflection && incidence_object_info->material.opacity < 1.0 && incidence_object_info->material.refraction_index > 0) {
        
        // Transmission ray
        Vector3 T = (N * -1.0) 
                    * 
                    sqrtf(
                        1.0 - ( powf(snells_ratio, 2.0)*(1.0-powf(cos_angle_incidence, 2.0)))
                    ) 
                    + 
                    (
                        ((N*cos_angle_incidence) - I)*snells_ratio
                    );

        Intersection min_intersection;
        SceneObjectInfo* intersected_object = nullptr; 
        float min_distance = std::numeric_limits<float>::max();
        for (auto & [object, intersections] : TraceRay(incidence_object_intersection.point, T)) 
        {   
            for (auto & intersection : intersections) 
            {  
                // Make distance greater than some small number here, likely the same intersection point.
                if (intersection.distance > environment.other["epsilon"] && intersection.distance < min_distance) {
                    
                    // Prevents self-intersections at the surface of faces, resulting in artifacts at edges of connected faces
                    if (incident_object_stack.size() > 0 && object->id != incident_object_stack.back()->id && incidence_object_info->type == "face") {
                       goto SKIP_TRANS;
                    } 

                    min_distance = intersection.distance;
                    
                    // The case when transmission ray is exiting the same material
                    min_intersection = intersection;
                    intersected_object = object;  
                }
            }
        }

        // Recurse on transmitted ray
        if (intersected_object != nullptr) {
        
            float new_incident_refraction_index;
            float new_transmittion_refraction_index;
            std::vector<SceneObjectInfo*> new_incident_object_stack = incident_object_stack;
            RayState new_ray_state;

            switch (previous_ray_state)
            {
            // Previous ray was entering an object...
            case RayState::ENTERING:
                //  ...and transmitted ray exits other side of sphere
                if (intersected_object->id == incidence_object_info->id) { 
                    new_ray_state = RayState::EXITING;
                    new_incident_refraction_index = new_incident_object_stack.back()->material.refraction_index;
                    new_incident_object_stack.pop_back();
                    new_transmittion_refraction_index = new_incident_object_stack.size() > 0 ? new_incident_object_stack.back()->material.refraction_index : environment.other["bkg_refraction_index"];
                    if (new_incident_object_stack.size() > 0) new_incident_object_stack.pop_back();

                // ...and transmitted ray enters into another internal material
                } else { 
                    new_ray_state = RayState::ENTERING;
                    new_incident_refraction_index = transmission_refraction_index;
                    new_transmittion_refraction_index = intersected_object->material.refraction_index;
                    new_incident_object_stack.push_back(intersected_object);
                }

                break;
            // Previous ray was exiting a material...
            case RayState::EXITING:
                if (new_incident_object_stack.size() > 0) {

                    // .. and transmissions enter a new object before exiting current media
                    if (!objectInStack(new_incident_object_stack, intersected_object)) { 
                        new_ray_state = RayState::ENTERING;
                        new_incident_refraction_index = transmission_refraction_index;
                        new_transmittion_refraction_index = intersected_object->material.refraction_index;
                        new_incident_object_stack.push_back(intersected_object);

                    // .. and transmission ray is exiting from nested containing material
                    } else { 
                        new_ray_state = RayState::EXITING;
                        new_incident_refraction_index = transmission_refraction_index;
                        new_transmittion_refraction_index = new_incident_object_stack.back()->material.refraction_index;
                        new_incident_object_stack.pop_back();
                    }  
                    
                // .. and transmission ray enters new object (through background space)
                } else { 
                    new_ray_state = RayState::ENTERING;
                    new_incident_refraction_index = environment.other["bkg_refraction_index"];
                    new_transmittion_refraction_index = intersected_object->material.refraction_index;
                    new_incident_object_stack = { intersected_object }; // We are not appending to stack if entering new obj from background 
                }
                
                break;
            }
        
            tmp_transparency = ShadeRay(
                T,
                intersected_object, 
                min_intersection, 
                new_incident_refraction_index,
                new_transmittion_refraction_index,
                new_incident_object_stack,
                new_ray_state,
                recursion_depth-1,
                background_color
            // ) * (1.0-F)*(expf(-1.0 * incidence_object_info->material.opacity*min_intersection.distance)); // Attenuated transparency via Beer's Law
            ) *(1.0-F)*(1.0 - incidence_object_info->material.opacity);
        
        // Use background color if no object was intersected
        } else {
            tmp_transparency = background_color * (1.0-F)*(1.0 - incidence_object_info->material.opacity);
        }
    }

    SKIP_TRANS:

    /*
        Determine contribution of pixel intensity from reflections:
        Simulate reflections using Schlick's approximation of Fresnel Reflectance.
        
        To do so, we recursively raytrace from the incidence ray "R" (Ray that is reflected off the surface of intersected object).
        This ray will bounce around the scene (traced up to "recursion_depth" times). 
        The RGB value returned by this traced ray is then multiplied by the Schlick approximation, "F",
        of the material surface's Fresnal reflectance.
    */
    
    F_0 = powf((incidence_object_info->material.refraction_index - 1)/(incidence_object_info->material.refraction_index + 1), 2.0); 
    F = F_0 + (1.0 - F_0)*powf(1.0 - (cos_angle_incidence), 5.0);
    if (recursion_depth > 0 && F != 0.0 && incidence_object_info->material.ks > 0.0) 
    {
        // Refraction ray
        Vector3 R = N*(2.0*(cos_angle_incidence)) - I;
        Intersection min_intersection;
        SceneObjectInfo* intersected_object = nullptr; 
        float min_distance = std::numeric_limits<float>::max();

        for (auto& [object, intersections] : TraceRay(incidence_object_intersection.point, R)) 
        {
            for (auto & intersection : intersections) 
            {
                if (intersection.distance > environment.other["epsilon"] && intersection.distance < min_distance) 
                {
                    min_distance = intersection.distance;
                    intersected_object = object;
                    min_intersection = intersection;
                }
            }
        }

        // Recurse on reflected ray
        if (intersected_object != nullptr)
        {
            float new_incident_refraction_index;
            float new_transmittion_refraction_index;
            std::vector<SceneObjectInfo*> new_incident_object_stack = incident_object_stack;
            RayState new_ray_state;

            switch (previous_ray_state)
            {
            // Previous ray was entering an object...
            case RayState::ENTERING:
                if (new_incident_object_stack.size() > 0) {

                    // .. and reflected ray enters new object before exiting parent media
                    if (!objectInStack(new_incident_object_stack, intersected_object)) { 
                        new_ray_state = RayState::ENTERING;
                        new_incident_refraction_index = incidence_refraction_index;
                        new_transmittion_refraction_index = intersected_object->material.refraction_index;
                        new_incident_object_stack.push_back(incidence_object_info);

                    // .. and reflected ray exits towards border of parent media
                    } else { 
                        new_ray_state = RayState::ENTERING;
                        new_incident_refraction_index = incidence_refraction_index;
                        new_transmittion_refraction_index = new_incident_object_stack.back()->material.refraction_index;
                        new_incident_object_stack.pop_back();
                    }

                // .. and reflect ray enters object in background media
                } else { 
                    new_ray_state = RayState::ENTERING;
                    new_incident_refraction_index = incidence_refraction_index;
                    new_transmittion_refraction_index = intersected_object->material.refraction_index;
                    new_incident_object_stack = { intersected_object };
                }
                
                break;
            
            // Previous ray was exiting an object...
            case RayState::EXITING: 
                // .. and reflected ray intersects back onto same surface
                if (intersected_object->id == incidence_object_info->id) {
                    new_ray_state = RayState::EXITING;
                    new_incident_refraction_index = incidence_refraction_index;
                    new_transmittion_refraction_index = transmission_refraction_index;

                // .. and reflected ray intersects a nested object
                } else {
                    new_ray_state = RayState::ENTERING;
                    new_incident_refraction_index = incidence_refraction_index;
                    new_transmittion_refraction_index = intersected_object->material.refraction_index;
                    new_incident_object_stack.push_back(intersected_object);
                }

                break;
            }

            tmp_reflection = ShadeRay(
                R,
                intersected_object, 
                min_intersection, 
                new_incident_refraction_index,
                new_transmittion_refraction_index,
                new_incident_object_stack,
                new_ray_state,
                recursion_depth-1,
                background_color
            ) * F;
            
        // Use background color if no object was intersected
        } else {
            tmp_reflection = background_color * F;
        }
    }
    
    
    /*
        Ambient + diffuse + specular + reflection + transparency
    */
    return (diffuse * material.ka) + tmp_specular + tmp_transparency + tmp_reflection;
}

/**
 * @brief Traces ray into scene, finding intersections with any and all scene objects.
 * @returns Returns a vector of intersection objects with points of intersection
 * @param ray Outgoing ray
 * @param view_origin origin of the ray
**/
std::vector<ObjectIntersections> TraceRay(Vector3 view_origin, Vector3 ray) 
{
    std::vector<ObjectIntersections> ray_trace_results;
    for ( auto& [type, object_infos] : environment.scene_object_infos) {
        if (type == "sphere") 
        {
            for (auto & object_info : object_infos) 
            {
                std::vector<Intersection> intersections;
                Sphere* sphere_object = environment.spheres[object_info->id];
                Vector3 dir = (view_origin - sphere_object->center);

                /*
                    Determine ray intersection points (x, y, z) via equation of sphere:
                    (x–sphere.x)^2 + (y–sphere.y)^2 + (z–sphere.z)^2 == sum[(intersection - sphere_center)^2] == sphere.r^2
                    intersection = view_origin + (distance * ray)
                */
                float A = 1.0;
                float B = 2.0 * (ray * dir).sum();
                float C = dir.square().sum() - pow(sphere_object->radius, 2.0);

                /*
                    When the sign of the determinant is Positive, there are two solutions.
                    When the sign of the determinant is Negative, there are no solutions.
                    When the determinant is Zero, there there is one solution.
                */
                
                float determinant = std::pow(B, 2.0) - (4.0 * A * C);
                if (!std::signbit(determinant)) {
                    float distance1 = (-B + std::sqrt(determinant)) / (2.0 * A);
                    Vector3 intersection1 = view_origin + (ray * distance1);
                    intersections.push_back({
                        distance1,
                        intersection1,
                        ((intersection1 - sphere_object->center) / sphere_object->radius).norm()
                    });
                    
                    float distance2 = (-B - std::sqrt(determinant)) / (2.0 * A);
                    Vector3 intersection2 = view_origin + (ray * distance2);
                    intersections.push_back({
                        distance2,
                        intersection2,
                        ((intersection2 - sphere_object->center) / sphere_object->radius).norm()
                    });
                    
                } else if (determinant == 0) {
                    float distance = -B / 2.0;
                    Vector3 intersection = view_origin + (ray * distance);
                    intersections.push_back({
                        distance,
                        intersection,
                        ((intersection - sphere_object->center) / sphere_object->radius).norm()
                    });
                }

                ObjectIntersections object_intersections;
                object_intersections.object_info = object_info;
                object_intersections.intersections = intersections;
                ray_trace_results.push_back(object_intersections);   
            }
        } else if (type == "face") {
            for (auto& object_info : object_infos) 
            {
                std::vector<Intersection> intersections; // Will only ever be one intersection per triangle (But other objects may differ)
                Face* face_object = environment.faces[object_info->id];
                Vector3 e1 = face_object->vertex[1] - face_object->vertex[0];
                Vector3 e2 = face_object->vertex[2] - face_object->vertex[0];
                Vector3 normal = face_object->surface_normal;

                /*
                    Determine if ray intersects plane that contains trangle:
                    - Ax + By + Cz + D = 0, equation of plane.
                    - A, B, and C are from triangle_normal.
                    - x, y, and z is any point on the plane.

                    From these, we can calculate:
                    D == -triangle_normal.dot(triangle_vertex)
                    distance_to_plane == -(triangle_normal.dot(view_origin) + D) / triangle_normal.dot(ray_direction)
                    ray_plane_intersection_point == view_origin + (distance_to_plane * ray_direction);
                */
               
                float dem = normal.dot(ray); 
                if (dem == 0.0f) {
                    continue; // We have missed the plane containing the triangle
                }
                
                float D = -normal.dot(face_object->vertex[0]);
                float distance = -(normal.dot(view_origin) + D) / dem;
                Vector3 intersection = view_origin + (ray * distance);
                
                /*
                    Determine if the intersection point lies within triangle:
                    - We do this by finding the Barycentric Coordinates of the triangle
                        
                        If p is some linear combination of p0, p1, and p2 (vertices of triangle), with weights a, b, and g, 

                            p = a * p0 + b * p1 + g * p2
    
                        given that

                            0 < a < 1 and 0 < b < 1 and 0 < g < 1
                            a + b + g = 1
                            a = 1 – ( b + g )

                        , then the point p is inside the triangle defined by p0, p1, and p2.

                    - For simplicity, we solve in terms of just b and g.

                        p = a * p0 + b * p1 + g * p2
                        p = (1 – b – g)p0 + b * p1 + g * p2
                        p = p0 + b(p1 – p0) + g(p2 – p0) 

                        ep = p – p0
                        e1 = p1 – p0
                        e2 = p2 – p0
                    
                        b * e1 + g * e2 = ep
                        b * e1 + g * e2 = ep

                    - Because the dot products of two vectors results in a scalar, we can furthure simplify the above two
                        equations by taking dot products using e1/e2 on both side of equations.

                        e1 . (b * e1 + g * e2) = e1.dot(ep) ==> b(e1 . e1) + g(e1 . e2) = e1 . ep
                        e2 . (b * e1 + g * e2) = e2.dot(ep) ==> b(e2 . e1) + g(e2 . e2) = e2 . ep

                    - now, just create variables for each dot product (d1 .. d6)

                        b*d1 + g*d2 = d5
                        b*d3 + g*d4 = d6

                    - And now we have just two equation with two unknowns we can solve,
                        Ax = b, 
                        A = [[d1, d2],
                             [d3, d4]]
                        b = [[d5],
                             [d6]]
                        x = [[b], 
                             [g]]

                        b = (d4*d5 – d2*d6)/(d1*d4 – d2*d3)
                        g = (d1*d6 – d2*d5)/(d1*d4 – d2*d3)
                */

                Vector3 ep;
                float d11, d22, d12, d1p, d2p, a, b, g;
                ep = intersection - face_object->vertex[0];
                d11 = e1.dot(e1);
                d12 = e1.dot(e2);
                d22 = e2.dot(e2);
                d1p = e1.dot(ep);
                d2p = e2.dot(ep);
                float det = (d11*d22 - d12*d12);
                if (det != 0.0f) {
                    b = (d22*d1p - d12*d2p) / det;
                    g = (d11*d2p - d12*d1p) / det;
                    a = 1.0f - ( b + g );

                    face_object->barycentric_cords = {
                        .x = a,
                        .y = b,
                        .z = g
                    };

                    if (((0.0f < a) && (a < 1.0f)) && ((0.0f < b) && ( b < 1.0f)) && ((0.0f < g) && (g < 1.0f))) {
                        Vector3 normal;
                        if (face_object->smooth_shading) {
                            normal = (
                                (face_object->vertex_normal[0].norm() * face_object->barycentric_cords.x) + 
                                (face_object->vertex_normal[1].norm() * face_object->barycentric_cords.y) + 
                                (face_object->vertex_normal[2].norm() * face_object->barycentric_cords.z)
                            ).norm();    
                        } else {
                            normal = face_object->surface_normal;
                        }

                        Intersection info;
                        info.distance = distance;
                        info.point = intersection;
                        info.normal = normal;

                        ObjectIntersections object_intersections = { 
                            .object_info = object_info,
                            .intersections = { info }
                        };
                        ray_trace_results.push_back(object_intersections);   
                    }
                }
            }
        }
    }
   
    return ray_trace_results;
}