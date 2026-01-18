#pragma once
#include <string>
#include <glm.hpp>
#include <core/input.hpp>
#include <GLFW/glfw3.h>

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
static void click_callback(GLFWwindow* window, int button, int action, int mods);
static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
static float scroll_mod(float delta, float power = 1.5f, float multiplier = 1.0f);

inline glm::vec2 screenToNDC(const glm::vec2& pos, const glm::vec2& resolution) {
    glm::vec2 ndc = (pos / resolution) * 2.0f - 1.0f;
    ndc.y = -ndc.y; // Invert Y to match OpenGL convention
    return ndc;
}

class Window {
public:
    int width;
    int height;
    glm::vec2 resolution;
    Window() {}
    Window(int width, int height, const char* title,int vSync = 1);
    ~Window();

    bool shouldClose() const;
    void swapBuffers();
    void pollEvents();
    void clear(float r, float g, float b, float a);
    void lock_mouse() {
        glfwSetInputMode((GLFWwindow*)window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
    void unlock_mouse() {
        glfwSetInputMode((GLFWwindow*)window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    // Optionally: expose access to the native GLFWwindow* if you need it internally
    GLFWwindow*getWindow() {return window;} // or GLFWwindow* getNativeWindow();
private:
    GLFWwindow* window; // Or, #include <GLFW/glfw3.h> and use GLFWwindow* window;
};

class Shader {
public:
    Shader(const char* vertexSource, const char* fragmentSource);
    ~Shader();

    void use();
    unsigned int getID() const;

    // Utility functions to set uniforms
    void setMat4(const char* name, const float* value);
    void setVec3(const char* name, float x, float y, float z);
    void setVec3(const char* name, glm::vec3 vec);
    void setVec4(const char* name, glm::vec4 vec);
    void setVec2(const char* name, glm::vec2 vec);
    void setInt(const char* name, int i);
    void setFloat(const char* name, float value);

    void setVec4Array(const char* name, const glm::vec4* data, int count) {
        for(int i = 0; i < count; i++) {
            std::string arrayName = std::string(name) + "[" + std::to_string(i) + "]";
            setVec4(arrayName.c_str(), data[i]);
        }
    }

    void setVec2Array(const char* name, const glm::vec2* data, int count) {
        for(int i = 0; i < count; i++) {
            std::string arrayName = std::string(name) + "[" + std::to_string(i) + "]";
            setVec2(arrayName.c_str(), data[i]);
        }
    }

    void setIntArray(const char* name, const int* data, int count) {
        for(int i = 0; i < count; i++) {
            std::string arrayName = std::string(name) + "[" + std::to_string(i) + "]";
            setInt(arrayName.c_str(), data[i]);
        }
    }

private:
    unsigned int ID;
};