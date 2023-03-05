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

class Mat3D {
private:
    size_t N, M, L;
    std::vector<int> m_data;

public:
    /*
        i ith row "y"
        j jth column "x"
        k kth entry in 3rd dim "z"

        Matrix N x M X K, where N is rows and M is columns
        Index at (i, j, k), where i is rows and j is columns
         
    */
    Mat3D(size_t m, size_t n, size_t l, int def = 0):
        N(n), M(m), L(l), m_data(n*m*l, def)
    {}
    
    int& operator()(size_t i, size_t j, size_t k) {
        return m_data.at(index(i, j, k));
    }

    int index(size_t i, size_t j, size_t k) {
        return M * L * i + L * j + k;
    }
};

struct Texture {
    public:
    float height, width;
    Texture(float height, float width) {
        this->width = width;
        this->height = height;
        image = new Mat3D(height, width, 3);
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
    float ka, kd, ks, n;
};

struct SceneObject 
{
    unsigned int id;
    std::string type; 
    Material material;
    Texture* texture = nullptr;
    bool has_texture; // Overrides material if present
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
    Point texture_coords[3];
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

#define RayTraceResults std::map<SceneObject*, std::vector<Intersection>>