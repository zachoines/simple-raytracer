#pragma once
#include <cmath>
#include <numeric>
#include <algorithm>
#include <string>
#include <stdexcept>

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

struct Texture {

};

struct SceneObject 
{
    unsigned int id;
    std::string type;
    Material material;
};

struct Sphere : SceneObject 
{
    Vector3 center;
    float radius;
};

struct Face : SceneObject 
{
    Vector3 surface_normal;
    Vector3 vertex[3];
    bool smooth_shading;
    Vector3 vertex_normal[3];
    bool has_texture;
    Texture texture;
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
