#pragma once


#include <glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <gtx/matrix_decompose.hpp>
#include <gtx/quaternion.hpp>
#include <fstream>
#include <sstream>
#include <string>

#include<util/basic.hpp>


#pragma once
#include <filesystem>
#include <string>
#include <stdexcept>

namespace Golden {
    inline bool looks_like_root(const std::filesystem::path& p) {
        return std::filesystem::exists(p / "Engine") &&
            std::filesystem::is_directory(p / "Engine") &&
            std::filesystem::exists(p / "CMakeLists.txt");
    }

    inline std::string find_root_from_cwd() {
        namespace fs = std::filesystem;
        fs::path cur = fs::absolute(fs::current_path());

        for (;;) {
            if (looks_like_root(cur)) return cur.string();
            if (cur == cur.root_path()) break;
            cur = cur.parent_path();
        }

        throw std::runtime_error("Golden root not found from current working directory");
    }


    inline std::string& root() {
        static std::string r = []{
            if (const char* env = std::getenv("GOLDEN_ROOT")) return std::string(env);
            return find_root_from_cwd();
        }();
        return r;
    }
}

inline glm::vec2 toNDC(const glm::vec2& pos, const glm::vec2& resolution) {
    glm::vec2 ndc = (pos / resolution) * 2.0f - 1.0f;
    ndc.y = -ndc.y; // Invert Y to match OpenGL convention
    return ndc;
}

class vec2 {
private:
    glm::vec2 impl; // the internal GLM implementation

public:
    vec2() : impl(0.0f, 0.0f) {}
    vec2(float x, float y) : impl(x, y) {}
    vec2(const glm::vec2& v) : impl(v) {}
    vec2(float z) : impl(z,z) {}

    float x() const { return impl.x; }
    float y() const { return impl.y; }

    void x(float x) { impl.x = x; }
    void y(float y) { impl.y = y; }

    vec2 addX(float x) {impl.x = impl.x+x; return vec2(impl.x,impl.y);}
    vec2 addY(float y) {impl.y = impl.y+y; return vec2(impl.x,impl.y);}
    vec2 nX() {impl.x=0; return vec2(impl.x,impl.y);}
    vec2 nY() {impl.y=0; return vec2(impl.x,impl.y);}
    vec2 setX(float x) {impl.x = x; return vec2(impl.x,impl.y);}
    vec2 setY(float y) {impl.y = y; return vec2(impl.x,impl.y);}

    void addXY(float x, float y) {impl.x = impl.x+x; impl.y = impl.y+y;} 
    void set(float x, float y) { impl.x = x; impl.y = y;}
    glm::vec2 toGlm() const { return impl; }

    vec2 operator*=(float by) {return vec2(impl.x*=by,impl.y*=by);     }
    vec2 operator*=(const vec2& rh) {impl*=rh.impl; return vec2(impl); }
    vec2 operator+=(const vec2& rh) {impl+=rh.impl; return vec2(impl); }
    vec2 operator-=(const vec2& rh) {impl-=rh.impl; return vec2(impl); }

    vec2 operator+(const vec2& rh) const { return vec2(impl + rh.impl); }
    vec2 operator-(const vec2& rh) const { return vec2(impl - rh.impl); }
    vec2 operator*(const vec2& rh) const { return vec2(impl * rh.impl); }
    vec2 operator*(float scalar) const { return vec2(impl * scalar); }
    vec2 operator/(float scalar) const { return vec2(impl / scalar); }
    vec2 operator/(const vec2& rh) const { return vec2(impl / rh.impl); }
    bool operator==(const vec2& rh) const {return impl == rh.impl;}
    bool operator!=(const vec2& rh) const {return impl != rh.impl;}

    float length() const { return glm::length(impl); }

    vec2 normalized() const {
        float len = length();
        if (len == 0.0f) return vec2(0,0);
        return vec2(impl / len);
    }
    float distance(const vec2& rh) const {return (rh - *this).length();}
    vec2 direction(const vec2& rh) const {return (rh - *this).normalized(); }
    float dot(const vec2& rh) const { return glm::dot(impl, rh.impl); }
    
    std::string to_string() {return std::to_string(x())+","+std::to_string(y());};

    void print() {std::cout << "(" << x() << "," << y() << ")" << std::endl;}
    void print() const {std::cout << "(" << x() << "," << y() << ")" << std::endl;}
};

class ivec2 {
private:
    glm::ivec2 impl; // the internal GLM implementation
public:
    ivec2() : impl(0, 0) {}
    ivec2(int x, int y) : impl(x, y) {}
    ivec2(const glm::ivec2& v) : impl(v) {}

    int x() const { return impl.x; }
    int y() const { return impl.y; }

    void x(int x) { impl.x = x; }
    void y(int y) { impl.y = y; }

    ivec2 addX(int x) { impl.x += x; return ivec2(impl.x, impl.y); }
    ivec2 addY(int y) { impl.y += y; return ivec2(impl.x, impl.y); }
    ivec2 nX() { impl.x = 0; return ivec2(impl.x, impl.y); }
    ivec2 nY() { impl.y = 0; return ivec2(impl.x, impl.y); }
    ivec2 setX(int x) { impl.x = x; return ivec2(impl.x, impl.y); }
    ivec2 setY(int y) { impl.y = y; return ivec2(impl.x, impl.y); }

    void addXY(int x, int y) { impl.x += x; impl.y += y; }
    void set(int x, int y) { impl.x = x; impl.y = y; }
    glm::ivec2 toGlm() const { return impl; }

    ivec2 operator+=(const ivec2& rh) { impl += rh.impl; return ivec2(impl); }
    ivec2 operator-=(const ivec2& rh) { impl -= rh.impl; return ivec2(impl); }
    ivec2 operator*=(const ivec2& rh) { impl *= rh.impl; return ivec2(impl); }
    ivec2 operator*=(int scalar)      { impl *= scalar; return ivec2(impl); }

    ivec2 operator+(const ivec2& rh) const { return ivec2(impl + rh.impl); }
    ivec2 operator-(const ivec2& rh) const { return ivec2(impl - rh.impl); }
    ivec2 operator*(const ivec2& rh) const { return ivec2(impl * rh.impl); }
    ivec2 operator*(int scalar) const      { return ivec2(impl * scalar); }
    ivec2 operator/(int scalar) const      { return ivec2(impl / scalar); }
    ivec2 operator/(const ivec2& rh) const { return ivec2(impl / rh.impl); }

    bool operator==(const ivec2& rh) const { return impl == rh.impl; }
    bool operator!=(const ivec2& rh) const { return impl != rh.impl; }

    // int dot(const ivec2& rh) const { return glm::dot(impl, rh.impl); }
    // int length2() const { return glm::dot(impl, impl); } // squared length
    float length() const { return glm::length(glm::vec2(impl)); } // promote to float
    ivec2 normalized() const {
        int len = length();
        if (len == 0.0f) return ivec2(0,0);
        return ivec2(impl / len);
    }

    std::string to_string() const { return std::to_string(x()) + "," + std::to_string(y()); }

    void print() const { std::cout << "(" << x() << "," << y() << ")" << std::endl; }
};
    

class vec3 {
    private:
        glm::vec3 impl; // the internal GLM implementation
    
    public:
    
        // Constructors
        vec3() : impl(0.0f, 0.0f, 0.0f) {}
        vec3(float x, float y, float z) : impl(x, y, z) {}
        vec3(const glm::vec3& v) : impl(v) {}
        vec3(const vec2& v2, float z) : impl(v2.x(),v2.y(),z) {}
        vec3(float n) : impl(n,n,n) {}
    
        // Accessors
        float x() const { return impl.x; }
        float y() const { return impl.y; }
        float z() const { return impl.z; }

        void x(float x) { impl.x = x; }
        void y(float y) { impl.y = y; }
        void z(float z) { impl.z = z; }
    
        //Quick additions
        //void addX(float x) { impl.x = impl.x+x;}
        //void addY(float y) { impl.y = impl.y+y;}
        //void addZ(float z) { impl.z = impl.z+z;}
    
        vec3 addX(float x) {impl.x = impl.x+x; return vec3(impl.x,impl.y,impl.z);}
        vec3 addY(float y) {impl.y = impl.y+y; return vec3(impl.x,impl.y,impl.z);}
        vec3 addZ(float z) {impl.z = impl.z+z; return vec3(impl.x,impl.y,impl.z);}
    
        vec3 nY() {impl.y=0; return vec3(impl.x,impl.y,impl.z);}
    
        void addXYZ(float x, float y, float z) {impl.x = impl.x+x; impl.y = impl.y+y;  impl.z = impl.z+z;} 
        //void add(vec3 other) {impl.x = impl.x+other.x(); impl.y = impl.y+other.y();  impl.z = impl.z+other.z();} 
    
        void set(float x, float y, float z) { impl.x = x; impl.y = y; impl.z = z; }
        vec3 setX(float x) {impl.x = x; return vec3(impl.x,impl.y,impl.z);}
        vec3 setY(float y) {impl.y = y; return vec3(impl.x,impl.y,impl.z);}
        vec3 setZ(float z) {impl.z = z; return vec3(impl.x,impl.y,impl.z);}
        // Conversion to glm::vec3
        glm::vec3 toGlm() const { return impl; }
    
        vec3 mult(float by) {return vec3(impl.x*by,impl.y*by,impl.z*by); };
    
        vec3 operator+=(const vec3& rhs) {impl+=rhs.impl; return vec3(impl); }
        vec3 operator-=(const vec3& rhs) {impl-=rhs.impl; return vec3(impl); }
        // Operators (example: addition)
        vec3 operator+(const vec3& rhs) const { return vec3(impl + rhs.impl); }
        vec3 operator-(const vec3& rhs) const { return vec3(impl - rhs.impl); }
        vec3 operator*(float scalar) const { return vec3(impl * scalar); }
        vec3 operator*(const vec3& rhs) const { return vec3(impl * rhs.impl); }
        vec3 operator*=(const vec3& rhs) {impl*=rhs.impl; return vec3(impl); }
        vec3 operator/(const vec3& rhs) const { return vec3(impl / rhs.impl); }
        vec3 operator/(float scalar) const { return vec3(impl / scalar); }
        bool operator==(const vec3& rhs) const {return impl == rhs.impl;}
        bool operator!=(const vec3& rhs) const {return impl != rhs.impl;}
    
        float length() const { return glm::length(impl); }
    
        vec3 normalized() const {
            float len = length();
            if (len == 0.0f) return vec3(0,0,0);
            return vec3(impl / len);
        }
        float distance(const vec3& rhs) const {return (rhs - *this).length();}
        vec3 direction(const vec3& rhs) const {return (rhs - *this).normalized(); }
        // Dot and cross as static or member methods
        float dot(const vec3& rhs) const { return glm::dot(impl, rhs.impl); }
        vec3 cross(const vec3& rhs) const { return vec3(glm::cross(impl, rhs.impl)); }
    
        std::string to_string() {return std::to_string(x())+","+std::to_string(y())+","+std::to_string(z());};
        void print() {std::cout << "(" << x() << "," << y() << "," << z() << ")" << std::endl;}
        void print() const {std::cout << "(" << x() << "," << y() << "," << z() << ")" << std::endl;}
    };

class vec4 {
    private:
        glm::vec4 impl;
    
    public:
        vec4() : impl(0.0f, 0.0f, 0.0f, 0.0f) {}
        vec4(float x, float y, float z, float w) : impl(x, y, z, w) {}
        vec4(const glm::vec4& v) : impl(v) {}
        vec4(const vec3& v3, float w) : impl(v3.x(), v3.y(), v3.z(), w) {}
    
        float x() const { return impl.x; }
        float y() const { return impl.y; }
        float z() const { return impl.z; }
        float w() const { return impl.w; }

        void x(float x) { impl.x = x; }
        void y(float y) { impl.y = y; }
        void z(float z) { impl.z = z; }
        void w(float w) { impl.w = w; }
    
        vec4 addX(float x) {impl.x = impl.x+x; return vec4(impl.x,impl.y,impl.z,impl.w);}
        vec4 addY(float y) {impl.y = impl.y+y; return vec4(impl.x,impl.y,impl.z,impl.w);}
        vec4 addZ(float z) {impl.z = impl.z+z; return vec4(impl.x,impl.y,impl.z,impl.w);}
        vec4 addW(float w) {impl.w = impl.w+w; return vec4(impl.x,impl.y,impl.z,impl.w);}
        vec4 nX() {impl.x=0; return vec4(impl.x,impl.y,impl.z,impl.w);}
        vec4 nY() {impl.y=0; return vec4(impl.x,impl.y,impl.z,impl.w);}
        vec4 nZ() {impl.z=0; return vec4(impl.x,impl.y,impl.z,impl.w);}
        vec4 nW() {impl.w=0; return vec4(impl.x,impl.y,impl.z,impl.w);}
        vec4 setX(float x) {impl.x = x; return vec4(impl.x,impl.y,impl.z,impl.w);}
        vec4 setY(float y) {impl.y = y; return vec4(impl.x,impl.y,impl.z,impl.w);}
        vec4 setZ(float z) {impl.z = z; return vec4(impl.x,impl.y,impl.z,impl.w);}
        vec4 setW(float w) {impl.w = w; return vec4(impl.x,impl.y,impl.z,impl.w);}
    
        void addXYZW(float x, float y, float z, float w) {impl.x = impl.x+x; impl.y = impl.y+y; impl.z = impl.z+z; impl.w = impl.w+w;}
        void set(float x, float y, float z, float w) { impl.x = x; impl.y = y; impl.z = z; impl.w = w;}
        glm::vec4 toGlm() const { return impl; }
    
        vec4 mult(float by) {return vec4(impl.x*by,impl.y*by,impl.z*by,impl.w*by);}
    
        vec4 operator*=(float by) {return vec4(impl.x*=by,impl.y*=by,impl.z*=by,impl.w*=by);}
        vec4 operator*=(const vec4& rh) {impl*=rh.impl; return vec4(impl);}
        vec4 operator+=(const vec4& rh) {impl+=rh.impl; return vec4(impl);}
        vec4 operator-=(const vec4& rh) {impl-=rh.impl; return vec4(impl);}
    
        vec4 operator+(const vec4& rh) const { return vec4(impl + rh.impl); }
        vec4 operator-(const vec4& rh) const { return vec4(impl - rh.impl); }
        vec4 operator*(const vec4& rh) const { return vec4(impl * rh.impl); }
        vec4 operator*(float scalar) const { return vec4(impl * scalar); }
        vec4 operator/(float scalar) const { return vec4(impl / scalar); }
        vec4 operator/(const vec4& rh) const { return vec4(impl / rh.impl); }

        bool operator==(const vec4& rhs) const {return impl == rhs.impl;}
        bool operator!=(const vec4& rhs) const {return impl != rhs.impl;}
    
        float length() const { return glm::length(impl); }
    
        vec4 normalized() const {
            float len = length();
            if (len == 0.0f) return vec4(0,0,0,0);
            return vec4(impl / len);
        }
        float distance(const vec4& rh) const {return (rh - *this).length();}
        vec4 direction(const vec4& rh) const {return (rh - *this).normalized();}
        float dot(const vec4& rh) const { return glm::dot(impl, rh.impl); }
    
        std::string to_string() {return std::to_string(x())+","+std::to_string(y())+","+std::to_string(z())+","+std::to_string(w());}
    
        void print() {std::cout << "(" << x() << "," << y() << "," << z() << "," << w() << ")" << std::endl;}
        void print() const {std::cout << "(" << x() << "," << y() << "," << z() << "," << w() << ")" << std::endl;}
    };


inline vec3 vec2_to_vec3(const vec2& vec) {
    return vec3(vec.x(),0,vec.y());
}
inline vec2 vec3_to_vec2(const vec3& vec) {
    return vec2(vec.x(),vec.y());
}
inline vec3 vec4_to_vec3(const vec4& vec) {
    return vec3(vec.x(),vec.y(),vec.z());
}
        
        
class mat4 {
    private:
        glm::mat4 impl;
    
    public:
        mat4() : impl(1.0f) {}
        mat4(float diagonal) : impl(diagonal) {}
        mat4(const glm::mat4& m) : impl(m) {}
        mat4(float m00, float m01, float m02, float m03,
                float m10, float m11, float m12, float m13,
                float m20, float m21, float m22, float m23,
                float m30, float m31, float m32, float m33) 
            : impl(m00, m01, m02, m03, m10, m11, m12, m13, m20, m21, m22, m23, m30, m31, m32, m33) {}
    
        vec4 getRow(int row) const { return vec4(impl[0][row], impl[1][row], impl[2][row], impl[3][row]); }
        vec4 getCol(int col) const { return vec4(impl[col]); }
        
        void setRow(int row, const vec4& v) { 
            impl[0][row] = v.x(); impl[1][row] = v.y(); 
            impl[2][row] = v.z(); impl[3][row] = v.w(); 
        }
        void setCol(int col, const vec4& v) { impl[col] = v.toGlm(); }
    
        float get(int row, int col) const { return impl[col][row]; }
        void set(int row, int col, float value) { impl[col][row] = value; }
    
        vec3 getPosition() const { return vec3(impl[3][0], impl[3][1], impl[3][2]); }
        void setPosition(const vec3& pos) { impl[3][0] = pos.x(); impl[3][1] = pos.y(); impl[3][2] = pos.z(); }
    
        mat4 translate(const vec3& translation) const { return mat4(glm::translate(impl, translation.toGlm())); }
        mat4 rotate(float angle, const vec3& axis) const { return mat4(glm::rotate(impl, angle, axis.toGlm())); }
        mat4 scale(const vec3& scaling) const { return mat4(glm::scale(impl, scaling.toGlm())); }
        mat4 scale(float scaling) const { return mat4(glm::scale(impl, glm::vec3(scaling))); }
    
        glm::mat4 toGlm() const { return impl; }
    
        mat4 operator+=(const mat4& rh) { impl += rh.impl; return mat4(impl); }
        mat4 operator-=(const mat4& rh) { impl -= rh.impl; return mat4(impl); }
        mat4 operator*=(const mat4& rh) { impl *= rh.impl; return mat4(impl); }
        mat4 operator*=(float scalar) { impl *= scalar; return mat4(impl); }
    
        mat4 operator+(const mat4& rh) const { return mat4(impl + rh.impl); }
        mat4 operator-(const mat4& rh) const { return mat4(impl - rh.impl); }
        mat4 operator*(const mat4& rh) const { return mat4(impl * rh.impl); }
        mat4 operator*(float scalar) const { return mat4(impl * scalar); }
        vec4 operator*(const vec4& v) const { return vec4(impl * v.toGlm()); }
    
        mat4 transposed() const { return mat4(glm::transpose(impl)); }
        mat4 inverse() const { return mat4(glm::inverse(impl)); }
        float determinant() const { return glm::determinant(impl); }
    
        static mat4 identity() { return mat4(1.0f); }
        static mat4 zero() { return mat4(0.0f); }
    
        std::string to_string() {
            std::string result = "";
            for(int row = 0; row < 4; row++) {
                result += "[";
                for(int col = 0; col < 4; col++) {
                    result += std::to_string(get(row, col));
                    if(col < 3) result += ",";
                }
                result += "]";
                if(row < 3) result += "\n";
            }
            return result;
        }
    
        void print() {std::cout << to_string() << std::endl;}
};

glm::mat4 interpolateMatrix(const glm::mat4& start, const glm::mat4& end, float alpha);

inline void translateMatrix(glm::mat4& mat,vec3 by) {
    mat = glm::translate(mat,by.toGlm());
}

inline void faceMatrixTo(glm::mat4& mat, const vec3& targetPos) {
    vec3 currentPos = vec3(mat[3]);
    vec3 toTarget = targetPos - currentPos;

    toTarget.addY(-toTarget.y()); // flatten to XZ plane
    if (toTarget.length() < 1e-5) return;

    float angle = atan2(toTarget.x(), toTarget.z());

    // Decompose current matrix into position, rotation, and scale
    glm::vec3 position = glm::vec3(mat[3]);
    glm::vec3 scale{
        glm::length(glm::vec3(mat[0])),
        glm::length(glm::vec3(mat[1])),
        glm::length(glm::vec3(mat[2]))
    };

    // Create rotation matrix around Y
    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0, 1, 0));

    // Apply scale after rotation
    rotation[0] *= scale.x;
    rotation[1] *= scale.y;
    rotation[2] *= scale.z;

    // Final transform: scaled & rotated, with position
    mat = rotation;
    mat[3] = glm::vec4(position, 1.0f);
}


// class BoundingBox {
// public:
//     glm::vec3 min;
//     glm::vec3 max;

//     BoundingBox()
//         : min(std::numeric_limits<float>::max()), max(std::numeric_limits<float>::lowest()) {}

//     BoundingBox(const glm::vec3& min, const glm::vec3& max)
//         : min(min), max(max) {}

//     void expandToInclude(const glm::vec3& point) {
//         min = glm::min(min, point);
//         max = glm::max(max, point);
//     }

//     vec3 extent() const {
//         return vec3(
//             max.x - min.x,
//             max.y - min.y,
//             max.z - min.z
//         );
//     }

//     char getLongestAxis() const {
//         glm::vec3 size = getSize(); 
        
//         if (size.x >= size.y && size.x >= size.z) return 'x';
//         if (size.y >= size.z) return 'y';
//         return 'z';
//     }

//     float volume() const {
//     glm::vec3 size = max - min;
//     return size.x * size.y * size.z;
//     }

//     void transform(const glm::mat4& matrix) {
//         glm::vec3 newMin(std::numeric_limits<float>::max());
//         glm::vec3 newMax(std::numeric_limits<float>::lowest());

//         for (int i = 0; i < 8; ++i) {
//             glm::vec3 corner = getCorner(i);
//             glm::vec3 transformed = glm::vec3(matrix * glm::vec4(corner, 1.0f));
//             newMin = glm::min(newMin, transformed);
//             newMax = glm::max(newMax, transformed);
//         }

//         min = newMin;
//         max = newMax;
//     }

//     glm::vec3 getCenter() const {
//         return (min + max) * 0.5f;
//     }

//     glm::vec3 getSize() const {
//         return max - min;
//     }

//     glm::vec3 getCorner(int index) const {
//         return glm::vec3(
//             (index & 1) ? max.x : min.x,
//             (index & 2) ? max.y : min.y,
//             (index & 4) ? max.z : min.z
//         );
//     }

//     BoundingBox& expand(float scalar) {
//         glm::vec3 center = getCenter();
//         glm::vec3 halfSize = getSize() * 0.5f;
//         glm::vec3 newHalfSize = halfSize * scalar;
        
//         min = center - newHalfSize;
//         max = center + newHalfSize;
//         return *this;
//     }

//     BoundingBox& expand(const BoundingBox& other) {
//         min = glm::min(min, other.min);
//         max = glm::max(max, other.max);
//         return *this;
//     }

//     bool intersects(const BoundingBox& other) const {
//         return (min.x <= other.max.x && max.x >= other.min.x) &&
//                 (min.y <= other.max.y && max.y >= other.min.y) &&
//                 (min.z <= other.max.z && max.z >= other.min.z);
//     }

//     bool contains(const glm::vec3& point) const {
//         return (point.x >= min.x && point.x <= max.x &&
//                 point.y >= min.y && point.y <= max.y &&
//                 point.z >= min.z && point.z <= max.z);
//     }
// };

class BoundingBox {
public:
    vec3 min;
    vec3 max;
    
    enum Dimension { DIM_2D, DIM_3D };
    Dimension dim;

    // Constructors
    BoundingBox(Dimension d = DIM_3D)
        : min(std::numeric_limits<float>::max(), 
                std::numeric_limits<float>::max(), 
                d == DIM_2D ? 0.0f : std::numeric_limits<float>::max()),
            max(std::numeric_limits<float>::lowest(),
                std::numeric_limits<float>::lowest(),
                d == DIM_2D ? 0.0f : std::numeric_limits<float>::lowest()),
            dim(d) {}

    BoundingBox(const vec3& _min, const vec3& _max, Dimension d = DIM_3D)
        : min(_min), max(_max), dim(d) {
        if (dim == DIM_2D) {
            min.z(0.0f);
            max.z(0.0f);
        }
    }
    
    // 2D convenience constructor
    BoundingBox(const vec2& _min, const vec2& _max)
        : min(_min.x(), _min.y(), 0.0f),
            max(_max.x(), _max.y(), 0.0f),
            dim(DIM_2D) {}

    void expandToInclude(const vec3& point) {
        min = vec3(
            std::min(min.x(), point.x()),
            std::min(min.y(), point.y()),
            dim == DIM_2D ? 0.0f : std::min(min.z(), point.z())
        );
        max = vec3(
            std::max(max.x(), point.x()),
            std::max(max.y(), point.y()),
            dim == DIM_2D ? 0.0f : std::max(max.z(), point.z())
        );
    }
    
    void expandToInclude(const vec2& point) {
        min.x(std::min(min.x(), point.x()));
        min.y(std::min(min.y(), point.y()));
        max.x(std::max(max.x(), point.x()));
        max.y(std::max(max.y(), point.y()));
    }

    vec3 extent() const {
        return vec3(
            max.x() - min.x(),
            max.y() - min.y(),
            dim == DIM_2D ? 0.0f : max.z() - min.z()
        );
    }

    char getLongestAxis() const {
        vec3 size = getSize();
        
        if (dim == DIM_2D) {
            return (size.x() >= size.y()) ? 'x' : 'y';
        }
        
        if (size.x() >= size.y() && size.x() >= size.z()) return 'x';
        if (size.y() >= size.z()) return 'y';
        return 'z';
    }

    float volume() const {
        vec3 size = max - min;
        if (dim == DIM_2D) {
            return size.x() * size.y(); // Area for 2D
        }
        return size.x() * size.y() * size.z();
    }

    void transform(const glm::mat4& matrix) {
        vec3 newMin(std::numeric_limits<float>::max());
        vec3 newMax(std::numeric_limits<float>::lowest());

        int cornerCount = dim == DIM_2D ? 4 : 8;
        for (int i = 0; i < cornerCount; ++i) {
            vec3 corner = getCorner(i);
            glm::vec3 transformed = glm::vec3(matrix * glm::vec4(corner.toGlm(), 1.0f));
            newMin = vec3(
                std::min(newMin.x(), transformed.x),
                std::min(newMin.y(), transformed.y),
                dim == DIM_2D ? 0.0f : std::min(newMin.z(), transformed.z)
            );
            newMax = vec3(
                std::max(newMax.x(), transformed.x),
                std::max(newMax.y(), transformed.y),
                dim == DIM_2D ? 0.0f : std::max(newMax.z(), transformed.z)
            );
        }

        min = newMin;
        max = newMax;
    }

    vec3 getCenter() const {
        return (min + max) * 0.5f;
    }
    
    vec2 getCenter2D() const {
        vec3 center = getCenter();
        return vec2(center.x(), center.y());
    }

    vec3 getSize() const {
        return max - min;
    }
    
    vec2 getSize2D() const {
        vec3 size = getSize();
        return vec2(size.x(), size.y());
    }

    vec3 getCorner(int index) const {
        if (dim == DIM_2D) {
            // 4 corners for 2D: index 0-3
            return vec3(
                (index & 1) ? max.x() : min.x(),
                (index & 2) ? max.y() : min.y(),
                0.0f
            );
        }
        // 8 corners for 3D
        return vec3(
            (index & 1) ? max.x() : min.x(),
            (index & 2) ? max.y() : min.y(),
            (index & 4) ? max.z() : min.z()
        );
    }

    BoundingBox& expand(float scalar) {
        vec3 center = getCenter();
        vec3 halfSize = getSize() * 0.5f;
        vec3 newHalfSize = halfSize * scalar;
        
        min = center - newHalfSize;
        max = center + newHalfSize;
        
        if (dim == DIM_2D) {
            min.z(0.0f);
            max.z(0.0f);
        }
        return *this;
    }

    vec3 closestPoint(const vec3& point) const {
        return vec3(
            std::clamp(point.x(), min.x(), max.x()),
            std::clamp(point.y(), min.y(), max.y()),
            dim == DIM_2D ? 0.0f : std::clamp(point.z(), min.z(), max.z())
        );
    }

    float distance(const vec3& point) const {
        vec3 closest = closestPoint(point);
        return (point - closest).length();
    }

    float distance(const BoundingBox& other) const {
        vec3 closest = closestPoint(other.getCenter());
        return other.distance(closest);
    }

    BoundingBox& expand(const BoundingBox& other) {
        min = vec3(
            std::min(min.x(), other.min.x()),
            std::min(min.y(), other.min.y()),
            dim == DIM_2D ? 0.0f : std::min(min.z(), other.min.z())
        );
        max = vec3(
            std::max(max.x(), other.max.x()),
            std::max(max.y(), other.max.y()),
            dim == DIM_2D ? 0.0f : std::max(max.z(), other.max.z())
        );
        return *this;
    }

    bool intersects(const BoundingBox& other) const {
        bool intersectsXY = (min.x() <= other.max.x() && max.x() >= other.min.x()) &&
                            (min.y() <= other.max.y() && max.y() >= other.min.y());
        
        if (dim == DIM_2D || other.dim == DIM_2D) {
            return intersectsXY; // Ignore Z for 2D
        }
        
        return intersectsXY &&
                (min.z() <= other.max.z() && max.z() >= other.min.z());
    }

    bool contains(const vec3& point) const {
        bool containsXY = (point.x() >= min.x() && point.x() <= max.x() &&
                            point.y() >= min.y() && point.y() <= max.y());
        
        if (dim == DIM_2D) {
            return containsXY;
        }
        
        return containsXY && (point.z() >= min.z() && point.z() <= max.z());
    }
    
    bool contains(const vec2& point) const {
        return (point.x() >= min.x() && point.x() <= max.x() &&
                point.y() >= min.y() && point.y() <= max.y());
    }
};

