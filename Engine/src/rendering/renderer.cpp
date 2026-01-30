#include <glad.h>
#include <rendering/renderer.hpp>
#include <iostream>
#include <GLFW/glfw3.h>
#include <stdexcept>


void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == -1)
        return; // Ignore keys not mapped
    if (action == GLFW_PRESS)
        Input::get().setKey(key, true);
    else if (action == GLFW_RELEASE)
        Input::get().setKey(key, false);

    // GLFW_REPEAT can be implmented here later
}

float scroll_mod(float delta, float power, float multiplier) {
    float sign = (delta >= 0) ? 1.0f : -1.0f;
    return sign * std::pow(std::abs(delta), power) * multiplier;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    float x = scroll_mod((float)xoffset, 1.3f, 0.25f);
    float y = scroll_mod((float)yoffset, 0.9f, 0.75f);
    Input::get().setScroll(x, y);
}

void click_callback(GLFWwindow* window, int button, int action, int mods) {

    if (button == -1)
        return; // Ignore keys not mapped
    if (action == GLFW_PRESS)
        Input::get().setKey(button, true);
    else if (action == GLFW_RELEASE)
        Input::get().setKey(button, false);
}

Window::Window(int width, int height, const char* title,int vSync) {
    if (!glfwInit())
        throw std::runtime_error("GLFW init failed!");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Required for macOS

    //Fullscreen mode
    // GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    // const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    // glfwWindowHint(GLFW_RED_BITS, mode->redBits);
    // glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
    // glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
    // glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
    // window = glfwCreateWindow(mode->width, mode->height, title, monitor, nullptr);

    window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    this->height = height;
    this->width = width;
    if (!window)
        throw std::runtime_error("Window creation failed!");

    glfwMakeContextCurrent((GLFWwindow*)window);
    glfwSetKeyCallback((GLFWwindow*)window,key_callback);
    glfwSetScrollCallback((GLFWwindow*)window, scroll_callback);
    glfwSetMouseButtonCallback((GLFWwindow*)window, click_callback);
    glfwSwapInterval(vSync); //Vsync 1=on 0=off
    //Use this for debuging later, somewhere else
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
    }

    resolution = glm::vec2(width * 2, height * 2);

}

Window::~Window() {
    glfwDestroyWindow((GLFWwindow*)window);
    glfwTerminate();
}

bool Window::shouldClose() const {
    return glfwWindowShouldClose((GLFWwindow*)window);
}

void Window::swapBuffers() {
    glfwSwapBuffers((GLFWwindow*)window);
}

void Window::pollEvents() {
    Input::get().advanceFrame();
    glfwPollEvents();
}


void Window::clear(float r, float g, float b, float a) {
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}


Shader::Shader(const char* vertexSource, const char* fragmentSource) {
    // Compile vertex shader
    unsigned int vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vertexSource, NULL);
    glCompileShader(vertex);
    
    int success;
    char infoLog[512];
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    unsigned int fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fragmentSource, NULL);
    glCompileShader(fragment);

    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    glLinkProgram(ID);
    glGetProgramiv(ID, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(ID, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

Shader::~Shader() {
    glDeleteProgram(ID);
}

void Shader::use() {
    glUseProgram(ID);
}

unsigned int Shader::getID() const {
    return ID;
}

void Shader::setMat4(const char* name, const float* value) {
    int loc = glGetUniformLocation(ID, name);
    glUniformMatrix4fv(loc, 1, GL_FALSE, value);
}

void Shader::setVec3(const char* name, float x, float y, float z) {
    int loc = glGetUniformLocation(ID, name);
    glUniform3f(loc, x, y, z);
}

void Shader::setVec3(const char* name, glm::vec3 vec) {
    int loc = glGetUniformLocation(ID, name);
    glUniform3f(loc, vec.x,vec.y,vec.z);
}

void Shader::setVec2(const char* name, glm::vec2 vec) {
    int loc = glGetUniformLocation(ID, name);
    glUniform2f(loc, vec.x,vec.y);
}

void Shader::setVec4(const char* name, glm::vec4 vec) {
    int loc = glGetUniformLocation(ID, name);
    glUniform4f(loc, vec.x,vec.y,vec.z,vec.w);
}

void Shader::setFloat(const char* name, float value) {
    int loc = glGetUniformLocation(ID, name);
    glUniform1f(loc, value);
}

void Shader::setInt(const char* name, int i) {
    int loc = glGetUniformLocation(ID, name);
    glUniform1i(loc, i);
}


