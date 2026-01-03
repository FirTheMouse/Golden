#pragma once

#include<tensor.hpp>


namespace Eigen {

// Utility to read big-endian integers
int32_t read_int(std::ifstream& file) {
    unsigned char bytes[4];
    file.read((char*)bytes, 4);
    return (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];
}

std::pair<g_ptr<tensor>, g_ptr<tensor>> load_mnist(
    const std::string& image_file,
    const std::string& label_file,
    int max_samples = -1  // -1 = load all
) {
    // Load images
    std::ifstream img_stream(image_file, std::ios::binary);
    if(!img_stream) throw std::runtime_error("Can't open " + image_file);
    
    int magic = read_int(img_stream);
    int num_images = read_int(img_stream);
    int rows = read_int(img_stream);
    int cols = read_int(img_stream);
    
    if(max_samples > 0) num_images = std::min(num_images, max_samples);
    
    auto images = make<tensor>(MatrixXf(num_images, rows * cols));
    
    for(int i = 0; i < num_images; i++) {
        for(int j = 0; j < rows * cols; j++) {
            unsigned char pixel;
            img_stream.read((char*)&pixel, 1);
            images->data_(i, j) = pixel / 255.0f;  // normalize to [0,1]
        }
    }
    
    // Load labels
    std::ifstream lbl_stream(label_file, std::ios::binary);
    if(!lbl_stream) throw std::runtime_error("Can't open " + label_file);
    
    magic = read_int(lbl_stream);
    int num_labels = read_int(lbl_stream);
    
    if(max_samples > 0) num_labels = std::min(num_labels, max_samples);
    
    auto labels = make<tensor>(MatrixXf::Zero(num_labels, 10));
    
    for(int i = 0; i < num_labels; i++) {
        unsigned char label;
        lbl_stream.read((char*)&label, 1);
        labels->data_(i, label) = 1.0f;  // one-hot encoding
    }
    
    return {images, labels};
}

unsigned int mnist_to_texture(g_ptr<tensor> images, int img_index) {
    // Extract one 28x28 image
    int w = 28, h = 28;
    unsigned char* data = new unsigned char[w * h * 4];  // RGBA
    
    for(int i = 0; i < h; i++) {
        for(int j = 0; j < w; j++) {
            // Flip vertically: read from bottom to top
            int flipped_i = (h - 1) - i;
            float val = images->data_(img_index, flipped_i * w + j);
            unsigned char pixel = (unsigned char)(val * 255.0f);
            
            int idx = (i * w + j) * 4;
            data[idx + 0] = pixel;
            data[idx + 1] = pixel;
            data[idx + 2] = pixel;
            data[idx + 3] = 255;
        }
    }
    
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, data);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);  // Nearest for pixel art look
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    delete[] data;
    return tex;
}

}