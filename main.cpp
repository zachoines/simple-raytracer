#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include "src/definitions.h"
#include "src/utility.h"


std::map<std::string, std::vector<SceneObjectInfo*>> scene_objects;
std::map<int, Face*> faces;
std::map<int, Sphere*> spheres;

/*
    Function hoisting
*/
Color ShadeRay(SceneObjectInfo* intersected_object_info, Intersection intersection, Vector3 view_origin, Vector3 view_direction, Material material, Vector3 world_location, std::vector<Light> scene_lights);
std::vector<ObjectIntersections> TraceRay(Vector3 view_origin, Vector3 ray);


/*
    material Material of the object hit by ray 
    world_location World location of intersection point of object and ray
    surface_normal The normal vector of the object where ray hits
    scene_lights All the lights currently in the scene
*/
Color ShadeRay(SceneObjectInfo* intersected_object_info, Intersection intersection, Vector3 view_origin, Vector3 view_direction, std::vector<Light> scene_lights) 
{
    Vector3 N = intersection.normal;
    Color tmp = { 0.0, 0.0, 0.0 };
    Color shadow_mask = { 1.0, 1.0, 1.0 };
    std::vector<bool> obstructions;
    
    Material material = intersected_object_info->material;
    Color diffuse;

    if (intersected_object_info->has_texture) 
    {
        if (intersected_object_info->type == "sphere") 
        {
            // Find texture coorinates
            float v = acos(intersection.normal.z()) / M_PI;
            float phi = atan2(intersection.normal.y(), intersection.normal.x());
            float u;
            u = map(phi, -M_PI, M_PI, 0.0, 1.0);
            // if (phi > 0.0) {
            //     u = phi / 2.0 * M_PI;
            // } else {
            //     u = (phi + (2.0*M_PI)) / (2.0 * M_PI);
            // }
            
            // Find texture pixel value that location
            Color new_diffuse;
            float width = static_cast<float>(intersected_object_info->texture->width);
            float height = static_cast<float>(intersected_object_info->texture->height);
            v = std::clamp<float>(v, 0.0, 1.0);
            u = std::clamp<float>(u, 0.0, 1.0);

            // TODO: Map using bi-linear interpolation 
            int i = static_cast<int>(std::clamp<float>(round((height - 1.0) * v), 0.0, height - 1.0));
            int j = static_cast<int>(std::clamp<float>(round((width - 1.0) * u), 0.0, width - 1.0));

            Mat3D* image = intersected_object_info->texture->image;
            new_diffuse.r = static_cast<float>(map(image->get(j, i, 0), MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, 0, 1.0));
            new_diffuse.g = static_cast<float>(map(image->get(j, i, 1), MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, 0, 1.0));
            new_diffuse.b = static_cast<float>(map(image->get(j, i, 2), MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, 0, 1.0));
           
            // Update diffuse color 
            diffuse = new_diffuse;
        } else if (intersected_object_info->type == "face") {
            Face* face = faces[intersected_object_info->id];
  
            float u = 
                (face->barycentric_cords.values[0]*std::clamp<float>(face->texture_coords[0].x, 0.0, 1.0)) +
                (face->barycentric_cords.values[1]*std::clamp<float>(face->texture_coords[1].x, 0.0, 1.0)) +
                (face->barycentric_cords.values[2]*std::clamp<float>(face->texture_coords[2].x, 0.0, 1.0));
            float v = 
                (face->barycentric_cords.values[0]*std::clamp<float>(face->texture_coords[0].y, 0.0, 1.0)) +
                (face->barycentric_cords.values[1]*std::clamp<float>(face->texture_coords[1].y, 0.0, 1.0)) +
                (face->barycentric_cords.values[2]*std::clamp<float>(face->texture_coords[2].y, 0.0, 1.0));


            v = std::clamp<float>(v, 0.0, 1.0);
            u = std::clamp<float>(u, 0.0, 1.0);
        
            // Find texture pixel value that location
            Color new_diffuse;
            float width = static_cast<float>(intersected_object_info->texture->width);
            float height = static_cast<float>(intersected_object_info->texture->height);
            
            // TODO: Map using bi-linear interpolation 
            int i = static_cast<int>(std::clamp<float>(round((width - 1.0f) * u), 0.0, width - 1.0));
            int j = static_cast<int>(std::clamp<float>(round((height - 1.0f) * v), 0.0, height - 1.0));

            Mat3D* image = intersected_object_info->texture->image;
            new_diffuse.r = static_cast<float>(
                map(
                    image->get(i, j, 0), 
                    MIN_PIXEL_VALUE, 
                    MAX_PIXEL_VALUE, 
                    0, 
                    1.0
                )
            );
            new_diffuse.g = static_cast<float>(
                map(
                    image->get(i, j, 1), 
                    MIN_PIXEL_VALUE, 
                    MAX_PIXEL_VALUE, 
                    0, 
                    1.0
                )
            );
            new_diffuse.b = static_cast<float>(
                map(
                    image->get(i, j, 2), 
                    MIN_PIXEL_VALUE, 
                    MAX_PIXEL_VALUE, 
                    0, 
                    1.0
                )
            );

            // Update diffuse color 
            diffuse = new_diffuse;
        }
    } 
    else 
    {
        diffuse = material.diffuse;
    }
    
    for (Light light : scene_lights) 
    {
        Vector3 L, H;
        bool obstructed = false;

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
            std::vector<ObjectIntersections> objects_intersections = TraceRay(intersection.point, ray);

            for ( auto [object, intersections] : objects_intersections) 
            {    
                if ((object->type == "sphere") || (object->type == "face"))
                {
                    /*
                        For spheres, we do not consider self intersections
                    */
                    if (object->id == intersected_object_info->id) 
                    {
                        continue;
                    }
                    
                    for (auto intersection : intersections) 
                    {
                        if (intersection.distance > 0.0f) 
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
 
            L = (light.position - intersection.point).norm();
            float distance_to_light = std::sqrt((intersection.point - light.position).square().sum());

            /*
                Determine if shadow exists:
                Ray-trace from point of intersection to light source, detecting other scene objects are occluding light.
            */
            std::vector<ObjectIntersections> objects_intersections = TraceRay(intersection.point, L);
          
            for ( auto [object, intersections] : objects_intersections) 
            {    
                if ((object->type == "sphere") || (object->type == "face")) 
                {
                    /*
                        For spheres, we do not consider self intersections
                    */
                    if (object->id == intersected_object_info->id) {
                        continue;
                    }
                    
                    /*
                        Find if object intersection is in-between object and light source
                    */ 
                    for (auto intersection : intersections) 
                    {
                        if (intersection.distance > 0.0f && intersection.distance < distance_to_light) 
                        {
                            shadow_mask = { 0.0, 0.0, 0.0 };
                            obstructed = true;
                        }
                    }    
                }
            }

            obstructions.push_back(obstructed);
        }
  
        Vector3 V = (intersection.point - view_origin).norm();
        H = ((L - V) / 2.0f).norm();
        Color diffuse_component = (diffuse * material.kd) * std::max(0.0f, N.dot(L)); // diffuse term 
        Color specular_component = (material.specular * material.ks) * powf(std::max(0.0f, N.dot(H)), material.n);
        tmp = tmp + (light.color * shadow_mask * (
            diffuse_component + 
            specular_component
        ));
    }
    
    /*
        Ambient + specular
    */         
    return (diffuse * material.ka) + tmp;    
}


/*
    Returns objectTraceRay and their intersection points with provided ray
*/
std::vector<ObjectIntersections> TraceRay(Vector3 view_origin, Vector3 ray) 
{
    std::vector<ObjectIntersections> ray_trace_results;
    for ( auto& [type, object_infos] : scene_objects) {
        if (type == "sphere") 
        {
            for (auto & object_info : object_infos) 
            {
                std::vector<Intersection> intersections;
                Sphere* sphere_object = spheres[object_info->id];
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
                Face* face_object = faces[object_info->id];
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

                    Vector3 barycentric_cords;
                    barycentric_cords.values[0] = a;
                    barycentric_cords.values[1] = b;
                    barycentric_cords.values[2] = g;
                    face_object->barycentric_cords = barycentric_cords;

                    if (((0.0f < a) && (a < 1.0f)) && ((0.0f < b) && ( b < 1.0f)) && ((0.0f < g) && (g < 1.0f))) {
                        Vector3 normal;
                        if (face_object->smooth_shading) {
                            normal = (
                                (face_object->vertex_normal[0].norm() * face_object->barycentric_cords.values[0]) + 
                                (face_object->vertex_normal[1].norm() * face_object->barycentric_cords.values[1]) + 
                                (face_object->vertex_normal[2].norm() * face_object->barycentric_cords.values[2])
                            ).norm();    
                        } else {
                            normal = face_object->surface_normal;
                        }
                        Intersection info;
                        info.distance = distance;
                        info.point = intersection;
                        info.normal = normal;

                        ObjectIntersections object_intersections;
                        object_intersections.object_info = object_info;
                        object_intersections.intersections = { info };
                        ray_trace_results.push_back(object_intersections);   
                    }
                }
            }
        }
    }
   
    return ray_trace_results;
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

    v x1 y1 z1
    f v1 v2 v3
*/
Mat3D create_view_window_and_ray_trace(Vector3 view_origin, Vector3 view_direction, Vector3 view_up, float fov_h, float res_h, float res_w, std::vector<Light> scene_lights, Color background_color) 
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
                pixel_color = ShadeRay(intersected_object, min_intersection, view_origin, view_direction, scene_lights);    
            }
            
            matt(i, j, 0) = static_cast<int>(map(pixel_color.r, 0, 1.0, MIN_PIXEL_VALUE, MAX_PIXEL_VALUE));
            matt(i, j, 1) = static_cast<int>(map(pixel_color.g, 0, 1.0, MIN_PIXEL_VALUE, MAX_PIXEL_VALUE));
            matt(i, j, 2) = static_cast<int>(map(pixel_color.b, 0, 1.0, MIN_PIXEL_VALUE, MAX_PIXEL_VALUE));
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
            if (object_types[x] == q){
                return true;
            }
        }
    } else {
        for(int x = 0; x < num_commands; x++){
            if (valid_commands[x] == q){
                return true;
            }
        }
    }

    return false;
}


/*
    Valid Args for config file:

    eye eyex eyey eyez                         (The location of the 'eye' within scene)
    viewdir vdirx  vdiry  vdirz                (Defines the direction the 'eye' is looking)
    updir upx  upy  upz                        (Roll of the camera)
    hfov fovh                                  (Horizonal field of view)
    imsize width  height                       (Output image dimentions)
    bkgcolor r  g  b                           (Scene background color) 
    mtlcolor Od Od Od Os Os Os ka kd ks n      (Material color)
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

int main(int argc,char* argv[])
{
    std::vector<Texture*> textures;
    std::vector<Light> scene_lights;
    std::map<std::string, std::vector<std::string>> commands;
    std::map<std::string, Point> texture_coords;
    std::map<unsigned int, Vector3> vertices;
    std::map<unsigned int, Vector3> normals;
    
    unsigned int obj_id_counter = 0;
    unsigned int vertex_counter = 0;
    unsigned int normal_counter = 0;
    unsigned int texture_coord_counter = 0;

    // Will toggle between these two when reading in commands
    Texture* current_texture = nullptr;
    Material current_material;
    bool use_texture = false;
    bool has_material = false;
    
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
                    // If not blank string or comment
                    if (!del.empty() || del.at(0) != '#') {
                        image_properties.push_back(del);
                    }
                }

                // If blank line or command without arguements
                if (image_properties.size() <= 1) {
                    continue;

                // If argument is a scene properties command
                } else if (check_args(image_properties[0])) {
                    std::string command = image_properties[0];
                    image_properties.erase(image_properties.begin());

                    /*
                        Because an object's color/texture is relative to previous command
                        'texture' will overwrite 'mtcolor', and vice versa.
                    */
                    if (command == "texture") {
                        Texture* texture = nullptr;
                        use_texture = true;
                        try
                        {
                            texture = read_texture(image_properties[0], texture);
                            current_texture = texture;
                            textures.push_back(texture);
                        }
                        catch(const std::exception& e)
                        {
                            std::cerr << e.what() << '\n';
                            std::cout << "ERROR: Issue reading 'texture' from ppm. Please verify." << std::endl;
                            return 0;
                        }
                    } else if (command == "mtlcolor") {
                        Material material;
                        use_texture = false;
                        try
                        {
                            material = read_material(image_properties);
                            current_material = material;
                            has_material = true;
                        }
                        catch(const std::exception& e)
                        {
                            std::cerr << e.what() << '\n';
                            std::cout << "ERROR: Issue parsing 'material' from arguments. Please verify." << std::endl;
                            return 0;
                        }
                    } else {
                        commands[command] = image_properties;
                    }
                
                // If arguement is a scene object
                } else if (check_args(image_properties[0], true)) {
                    obj_id_counter++;
                    if (image_properties[0] == "sphere") 
                    {
                        Sphere* sphere_object = new Sphere();
                        SceneObjectInfo* object_info = new SceneObjectInfo();
                        try
                        {
                            object_info->id = obj_id_counter;
                            object_info->type = "sphere";
                            sphere_object->radius = std::stof(image_properties[4]);
                            sphere_object->center = {
                                {std::stof(image_properties[1]), std::stof(image_properties[2]), std::stof(image_properties[3])}
                            };
                        }
                        catch(const std::exception& e)
                        {
                            std::cerr << e.what() << '\n';
                            std::cout << "ERROR: Invalid args for 'sphere' object. Please verify." << std::endl;
                            return 0;
                        }

                        try
                        {
                            object_info->material = current_material;

                            if (use_texture) {
                                if (!has_material || current_texture == nullptr) {
                                    std::cout << "ERROR: Must define a 'mtlcolor' and 'texture'. Please verify." << std::endl;
                                    return 0; 
                                }
                                object_info->texture = current_texture;
                                object_info->has_texture = true;
                            } else {
                                if (!has_material) {
                                    std::cout << "ERROR: Must define a 'mtlcolor'. Please verify." << std::endl;
                                    return 0; 
                                }
                                object_info->has_texture = false;
                            }    
                        }
                        catch(const std::exception& e)
                        {
                            std::cerr << e.what() << '\n';
                            std::cout << "ERROR: Invalid args for 'mtlcolor' command. Please verify." << std::endl;
                            return 0;
                        }
                        
                        sphere_object->object_info = object_info;
                        scene_objects["sphere"].push_back(
                            object_info
                        );
                        spheres[object_info->id] = sphere_object;
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
                            std::cerr << e.what() << '\n';
                            std::cout << "ERROR: Invalid args for 'light' command. Please verify." << std::endl;
                            return 0;
                        }
                        
                        scene_lights.push_back(light);
                    } else if (image_properties[0] == "v") {
                        /*
                            Extract Vertex. Validate Correctness.
                        */
                        try
                        {
                            vertex_counter++;
                            Vector3 vertex;
                            vertex.values[0] = std::stof(image_properties[1]); 
                            vertex.values[1] = std::stof(image_properties[2]);
                            vertex.values[2] = std::stof(image_properties[3]);
                            vertices[vertex_counter] = vertex;
                            
                        }
                        catch(const std::exception& e)
                        {
                            std::cerr << e.what() << '\n';
                            std::cout << "ERROR: Invalid args for vertex. Please verify." << std::endl;
                            return 0;
                        }
                    } else if (image_properties[0] == "vn") {
                        /*
                            Extract Vertex normal. Validate Correctness.
                        */
                        try
                        {
                            normal_counter++;
                            Vector3 vertex_normal;
                            vertex_normal.values[0] = std::stof(image_properties[1]); 
                            vertex_normal.values[1] = std::stof(image_properties[2]);
                            vertex_normal.values[2] = std::stof(image_properties[3]);
                            normals[normal_counter] = vertex_normal;
                        }
                        catch(const std::exception& e)
                        {
                            std::cerr << e.what() << '\n';
                            std::cout << "ERROR: Invalid args for vertex normal. Please verify." << std::endl;
                            return 0;
                        }
                    } else if (image_properties[0] == "f") {
                        /*
                            Extract Face. Validate Correctness.
                        */
                        Face* triangle = new Face();
                        SceneObjectInfo* object_info = new SceneObjectInfo();
                        obj_id_counter++;
                        object_info->type = "face";
                        object_info->id = obj_id_counter; 
                        
                        try
                        {
                            for (int i = 1; i <= 3; i++) {
                                // Parse for vertex normals and/or texture coordinates
                                unsigned int t; // Texture coord
                                unsigned int n; // Normal
                                unsigned int v; // Vertex
                                if (sscanf(image_properties[i].c_str(), "%d/%d/%d", &v, &t, &n ) == 3) {
                                    // success reading a face in v/t/n format. For a smooth shaded, textured triangle.
                                    triangle->vertex[i - 1] = vertices[v];
                                    triangle->vertex_normal[i - 1] = normals[n];
                                    triangle->smooth_shading = true;
                                    triangle->texture_coords[i - 1] = texture_coords[std::to_string(t)];
                                    object_info->has_texture = true;

                                } else if (sscanf(image_properties[i].c_str(), "%d//%d", &v, &n ) == 2) {
                                    //success reading a face in v//n format. For a smooth shaded, untextured triangle.
                                    triangle->vertex[i - 1] = vertices[v];
                                    triangle->vertex_normal[i - 1] = normals[n];
                                    triangle->smooth_shading = true;
                                    object_info->has_texture = false;

                                } else if (sscanf(image_properties[i].c_str(), "%d/%d", &v, &t) == 2) {
                                    // success reading a face in v/t format. For a non-smooth shaded, textured triangle.
                                    triangle->vertex[i - 1] = vertices[v];
                                    triangle->smooth_shading = false;
                                    triangle->texture_coords[i - 1] = texture_coords[std::to_string(t)];
                                    object_info->has_texture = true;
                                
                                } else if (sscanf(image_properties[i].c_str(), "%d", &v) == 1) {
                                    // success reading a face in v format; proceed accordingly
                                    triangle->vertex[i - 1] = vertices[v];
                                    triangle->smooth_shading = false;
                                    object_info->has_texture = false;                               
                                } else {
                                    // error reading face data
                                    std::cout << "ERROR: Invalid args for 'f' object. Please verify." << std::endl;
                                    return 0;
                                }
                            };

                            // Enter material/ texture information information
                            try
                            {
                                object_info->material = current_material;

                                if (use_texture) {
                                    if (!has_material || current_texture == nullptr) {
                                        std::cout << "ERROR: Must define a 'mtlcolor' and 'texture'. Please verify." << std::endl;
                                        return 0; 
                                    }
                                    object_info->texture = current_texture;
                                    object_info->has_texture = true;
                                } else {
                                    if (!has_material) {
                                        std::cout << "ERROR: Must define a 'mtlcolor'. Please verify." << std::endl;
                                        return 0; 
                                    }
                                    object_info->has_texture = false;
                                }
                            }
                            catch(const std::exception& e)
                            {
                                std::cerr << e.what() << '\n';
                                std::cout << "ERROR: Invalid args for 'mtlcolor' (material color) command. Please verify." << std::endl;
                                return 0;
                            }

                            // Calculate main surface normal
                            Vector3 e1 = triangle->vertex[1] - triangle->vertex[0];
                            Vector3 e2 = triangle->vertex[2] - triangle->vertex[0];
                            triangle->surface_normal = e1.cross(e2).norm();
                            triangle->object_info = object_info;
                            faces[object_info->id] = triangle; 
                            scene_objects["face"].push_back(
                                object_info
                            );
                        }
                        catch(const std::exception& e)
                        {
                            std::cout << "ERROR: Invalid args for 'f' (face) object. Please verify." << std::endl;
                            return 0;
                        }
                    } else if (image_properties[0] == "vt") {
                        
                        /*
                            Extract texture coordinate. Validate Correctness.
                        */
                        try
                        {
                            Point coord;
                            coord.x = std::stof(image_properties[1]); // x coord
                            coord.y = std::stof(image_properties[2]); // y coord

                            texture_coord_counter++;
                            texture_coords[std::to_string(texture_coord_counter)] = coord;
                        }
                        catch(const std::exception& e)
                        {
                            std::cerr << e.what() << '\n';
                            std::cout << "ERROR: Invalid args for texture coordinate. Please verify." << std::endl;
                            return 0;
                        }
                    } else {
                        std::cout << "WARNING: invalid argument '" << image_properties[0] << "' found in config. " << "Please verify path." << std::endl;
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
                std::cerr << e.what() << '\n';
                std::cout << "ERROR: Invalid image dimensions. Please verify." << std::endl;
                return 0;
            }

            try
            {
                width = std::stoi(commands.at("imsize")[0]);
            }
            catch(const std::exception& e)
            {
                std::cerr << e.what() << '\n';
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
                std::cerr << e.what() << '\n';
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
                std::cerr << e.what() << '\n';
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
                std::cerr << e.what() << '\n';
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
                std::cerr << e.what() << '\n';
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
                std::cerr << e.what() << '\n';
                std::cout << "ERROR: Invalid args for 'bkgcolor' command. Please verify." << std::endl;
                return 0;
            }
        }

        /*
            Using previous commands, build scene viewing window and raytrace.
        */
        Mat3D matt = create_view_window_and_ray_trace(view_origin, view_direction.norm(), view_up.norm(), fov_h, height, width, scene_lights, background_color); 


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
                    std::cerr << e.what() << '\n';
                }  
            }
        }

        image_stream.close();

    } else {
        std::cout << "Error: Incorrect number of arguements in input file. Please follow this formate: imsize width height" << std::endl;
    }

    return 0;
}