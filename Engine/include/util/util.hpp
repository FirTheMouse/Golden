#pragma once
#include <unordered_map>
#include <any>
#include <stdlib.h>
#include <unistd.h>
#include <initializer_list>
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>
#include <fstream>
#include <sstream>
#include <string>
// #include <util/q_list.hpp>
#include <util/list.hpp>
#include <util/map.hpp>
// #include <util/q_map.hpp>
#include<util/basic.hpp>

inline list<std::string> split_str(const std::string& s,char delimiter)
{
    list<std::string> toReturn;
    int last = 0;
    for(int i=0;i<s.length();i++)
    {
        if(s.at(i)==delimiter) {
            toReturn << s.substr(last,i-last);
            last = i+1;
        }
    }
    if(last<s.length())
    {
        toReturn << s.substr(last,s.length()-last);
    }
    return toReturn;
}

class Data{
public:
    Data() {}

    map<std::string,std::any> notes;

    template<typename T = std::string>
    void add(const std::string& label,T info)
    {
        notes.put(label,std::any(info));
    }

    template<typename T = std::string>
    T get(const std::string& label)
    {
        if(!has(label)) std::cerr << "Data does not have label " << label <<"\n";
        return std::any_cast<T>(notes.get(label));
    }

    bool has(const std::string& label)
    {
        return notes.hasKey(label);
    }

    bool check(const std::string& label)
    {
        if(!has(label)) return false;
        try {
            return get<bool>(label);
        }
        catch(std::exception e)
        {
            print("data::check::59 Attempted to check a non-bool in data");
            //Or just return false?
            return false;
        }
    }

    bool toggle(const std::string& label) {
        if(!has(label)) set<bool>(label,true);
        bool toReturn = !get<bool>(label);
        set<bool>(label,toReturn);
        return toReturn;
    }

    void flagOn(const std::string& label) {set<bool>(label,true);}
    void flagOff(const std::string& label) {set<bool>(label,false);}

    template<typename T>
    void set(const std::string& label,T info) {
        if(!notes.set(label,info))
            add<T>(label,info);
    }

    template<typename T = int>
    T inc(const std::string& label,T by)
    {
        if(has(label)) {set<T>(label,get<T>(label)+by);}
        else {add<T>(label,by);}
        return get<T>(label);
    }

    void debugData() {
        notes.debugMap();
    }
    
    /// @brief Scans through based on provided list, returns all missing labels
    list<std::string> validate(list<std::string> toCheck)
    {
        list<std::string> toReturn;
        for(auto s : toCheck) if(!has(s)) toReturn << s;
        return toReturn;
    }

    // void print() const
    // {
        
    // }

};

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

    float x() const { return impl.x; }
    float y() const { return impl.y; }

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

    bool operator==(const ivec2& rh) const { return impl == rh.impl; }
    bool operator!=(const ivec2& rh) const { return impl != rh.impl; }

    // int dot(const ivec2& rh) const { return glm::dot(impl, rh.impl); }
    // int length2() const { return glm::dot(impl, impl); } // squared length
    float length() const { return glm::length(glm::vec2(impl)); } // promote to float

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
    
        // Accessors
        float x() const { return impl.x; }
        float y() const { return impl.y; }
        float z() const { return impl.z; }
    
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
        vec3 operator/(const vec3& rhs) const { return vec3(impl / rhs.impl); }
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


inline std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) throw std::runtime_error("Could not open file: " + filename);
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

glm::mat4 interpolateMatrix(const glm::mat4& start, const glm::mat4& end, float alpha);

