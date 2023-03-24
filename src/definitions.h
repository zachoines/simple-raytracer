#pragma once
#include <cmath>
#include <numeric>
#include <algorithm>
#include <string>
#include <stdexcept>
#include "config.h"
struct Point {
    float x;
    float y;
};
struct Vector3 
{
    float x;
    float y;
    float z;

    float sum() 
    {
        return x + y + z;
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
        result.x *= result.x;
        result.y *= result.y;
        result.z *= result.z;
        return result;
    } 

    Vector3 cross (Vector3 other) 
    {
        Vector3 result;
        result.x=this->y*other.z - this->z*other.y;
        result.y=this->z*other.x - this->x*other.z;
        result.z=this->x*other.y - this->y*other.x;
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
        result.x *= other;
        result.y *= other;
        result.z *= other;
        return result;
    }

    Vector3 operator * (Vector3 other) 
    {
        Vector3 result = *this;
        result.x *= other.x;
        result.y *= other.y;
        result.z *= other.z;
        return result;
    }

    Vector3 operator / (float other) 
    {
        Vector3 result = *this;
        result.x /= other;
        result.y /= other;
        result.z /= other;
        return result;
    }

    Vector3 operator / (Vector3 other) 
    {
        Vector3 result = *this;
        result.x /= other.x;
        result.y /= other.y;
        result.z /= other.z;
        return result;
    }

    Vector3 operator + (Vector3 other) 
    {
        Vector3 result = *this;
        result.x += other.x;
        result.y += other.y;
        result.z += other.z;
        return result;
    }

    Vector3 operator - (Vector3 other) 
    {
        Vector3 result = *this;
        result.x -= other.x;
        result.y -= other.y;
        result.z -= other.z;
        return result;
    }

    Vector3 operator - (float other) 
    {
        Vector3 result = *this;
        result.x -= other;
        result.y -= other;
        result.z -= other;
        return result;
    }

    float operator [] (int index) 
    {
        if (index < 0 || index > 2) {
            throw std::out_of_range("Vector3 only can be index from 0 to 2");
        }
        if (index == 0) {
            return x;
        } else if (index == 1) {
            return y;
        } else {
            return z;
        }
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

    Color operator + (float other) 
    {
        Color result = *this;
        result.r = std::clamp(other + result.r, 0.0f, 1.0f);
        result.g = std::clamp(other + result.g, 0.0f, 1.0f);
        result.b = std::clamp(other + result.b, 0.0f, 1.0f);
        return result;
    }

    Color operator + (Vector3 from) {
        return *this + Color({
            .r = from.x,
            .g = from.y,
            .b = from.z
        });
    }
};

class Mat3D {
private:
    size_t N, M, L;
    std::vector<std::vector<std::vector<size_t>>> m_data;

public:
    /*
        i ith row "y"
        j jth column "x"
        k kth entry in 3rd dim "z"

        Matrix N x M X K, where N is rows and M is columns
        Index at (i, j, k), where i is rows and j is columns
    */
    Mat3D(size_t m, size_t n, size_t l, int def = 0):
        N(n), M(m), L(l), m_data(m, std::vector<std::vector<size_t>>(n, std::vector<size_t>(l,def)))
    {}
    
    size_t& operator()(size_t i, size_t j, size_t k) {
        return m_data[i][j][k];
    }

    void set(size_t i, size_t j, size_t k, size_t value) {
        m_data[i][j][k] = value;
    }

    size_t get(size_t i, size_t j, size_t k) {
        return m_data[i][j][k];
    }
};

struct Texture {
    public:
    float height, width;
    Texture(float width, float height) {
        this->width = width;
        this->height = height;
        image = new Mat3D(width, height, 3);
    }
    Mat3D* image;
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
    float ka, kd, ks, n, opacity, refraction_index;
};

struct SceneObjectInfo 
{
    unsigned int id;
    std::string type; 
    Material material;
    Texture* texture = nullptr;
    bool has_texture; // Overrides material if present
};

struct Sphere
{
    Vector3 center;
    float radius;
    SceneObjectInfo* object_info;
};

struct Face 
{
    Vector3 surface_normal;
    Vector3 vertex[3];
    bool smooth_shading;
    Vector3 vertex_normal[3];
    Vector3 barycentric_cords;
    Point texture_coords[3];
    SceneObjectInfo* object_info;
};

struct Light
{
    Vector3 position; // For positional lights
    Vector3 direction; // For directional lights
    float w;
    Color color;
};

struct Intersection
{
    float distance;
    Vector3 point;
    Vector3 normal;
};

struct ObjectIntersections 
{
    SceneObjectInfo* object_info;
    std::vector<Intersection> intersections;
};

struct Globals {
    std::map<std::string, std::vector<SceneObjectInfo*>> scene_object_infos;
    std::map<int, Face*> faces;
    std::map<int, Sphere*> spheres;
    std::vector<Light> scene_lights;
    std::map<std::string, std::vector<std::string>> commands;
    std::map<std::string, float> other;
};