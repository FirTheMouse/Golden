#pragma once

#include<util/ml_core.hpp>
#include<gui/twig.hpp>

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
    
    // Create tensor with shape [num_images, rows * cols]
    auto images = make<tensor>(list<int>{num_images, rows * cols});
    
    for(int i = 0; i < num_images; i++) {
        for(int j = 0; j < rows * cols; j++) {
            unsigned char pixel;
            img_stream.read((char*)&pixel, 1);
            images->at({i, j}) = pixel / 255.0f;  // normalize to [0,1]
        }
    }
    
    // Load labels
    std::ifstream lbl_stream(label_file, std::ios::binary);
    if(!lbl_stream) throw std::runtime_error("Can't open " + label_file);
    
    magic = read_int(lbl_stream);
    int num_labels = read_int(lbl_stream);
    
    if(max_samples > 0) num_labels = std::min(num_labels, max_samples);
    
    // Create tensor with shape [num_labels, 10] for one-hot encoding
    auto labels = make<tensor>(list<int>{num_labels, 10});
    labels->fill(0.0f);  // Initialize to zeros
    
    for(int i = 0; i < num_labels; i++) {
        unsigned char label;
        lbl_stream.read((char*)&label, 1);
        labels->at({i, (int)label}) = 1.0f;  // one-hot encoding
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
            float val = images->at({img_index, flipped_i * w + j});
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

void run_mnist(g_ptr<Scene> scene, int epochs, int amt = -1) {
    auto source_code = make<Font>(root()+"/Engine/assets/fonts/source_code_black.ttf",100);

    auto [train_imgs, train_labels] = load_mnist(
        root()+"/Projects/FirML/assets/images/train-images-idx3-ubyte", 
        root()+"/Projects/FirML/assets/images/train-labels-idx1-ubyte", 
        amt
    );
    auto [test_imgs, test_labels] = load_mnist(
        root()+"/Projects/FirML/assets/images/t10k-images-idx3-ubyte", 
        root()+"/Projects/FirML/assets/images/t10k-labels-idx1-ubyte", 
        amt
    );
    print("Loaded ", train_imgs->shape_[0], " training images");

    // Initialize weights and biases
    auto w1 = weight(784, 128, 0.1f);
    auto b1 = bias(128);
    auto w2 = weight(128, 10, 0.1f);
    auto b2 = bias(10);

    // Network definition - now using ADD instead of ADD_BIAS
    list<Pass> network = {
        {MATMUL, w1}, 
        {ADD, b1},      // Changed from ADD_BIAS to ADD
        {RELU},
        {MATMUL, w2}, 
        {ADD, b2}       // Changed from ADD_BIAS to ADD
    };
    list<g_ptr<tensor>> params = {w1, b1, w2, b2};

    Log::Line l;
    l.start();

    g_ptr<t_config> ctx = make<t_config>();
    ctx->epochs = epochs;
    ctx->learning_rate = 0.64f;
    ctx->grad_clip = 1.0f;
    ctx->reduction = MEAN;
    ctx->batch_size = 64;

    train_network(
        train_imgs,      // Starting points
        train_labels,    // Targets for the network
        network,         // Nodes
        params,          // Tensors
        SOFTMAX_CE,      // Loss type
        ctx,
        1                // Logging interval (if 0, turns off logging)
    );

    double time = l.end();
    print("Took ", time/1000000, "ms");

    // Run forward pass on test data
    auto test_output = test_imgs;
    for(auto p : network) {
        test_output = test_output->forward(p);
    }

    // Calculate accuracy
    int correct = 0;
    int num_test = test_output->shape_[0];
    int num_classes = test_output->shape_[1];
    
    for(int i = 0; i < num_test; i++) {
        // Find predicted class (max value in row)
        int pred_class = 0;
        float max_val = test_output->at({i, 0});
        for(int j = 1; j < num_classes; j++) {
            if(test_output->at({i, j}) > max_val) {
                max_val = test_output->at({i, j});
                pred_class = j;
            }
        }
        
        // Find true class (which column is 1 in one-hot encoding)
        int true_class = 0;
        for(int j = 0; j < num_classes; j++) {
            if(test_labels->at({i, j}) == 1.0f) {
                true_class = j;
                break;
            }
        }
        
        if(pred_class == true_class) correct++;
    }

    float accuracy = 100.0f * correct / num_test;
    print("Test Accuracy: ", accuracy, "%");

    // Display random predictions
    list<int> indices = {
        randi(0, test_imgs->shape_[0] - 1), 
        randi(0, test_imgs->shape_[0] - 1), 
        randi(0, test_imgs->shape_[0] - 1)
    };

    list<g_ptr<Text>> twigs;
    for(int i = 0; i < 3; i++) {
        int idx = indices[i];
        
        // Display the image
        g_ptr<Quad> q = make<Quad>();
        scene->add(q);
        q->setTexture(mnist_to_texture(test_imgs, idx));
        float pos_y = (float)i * 300.0f;
        vec2 pos(pos_y, 0.0f);
        q->setPosition(pos);
        vec2 scale(280, 280);
        q->scale(scale);

        // Get prediction for single image
        auto single_img = test_imgs->get_batch(idx, 1);
        auto output = single_img;
        for(auto p : network) {
            output = output->forward(p);
        }
        
        // Find predicted class (max output)
        int predicted = 0;
        for(int j = 1; j < num_classes; j++) {
            if(output->at({0, j}) > output->at({0, predicted})) {
                predicted = j;
            }
        }
        
        // Display text showing prediction
        print("Image ", i, ": Predicted ", predicted);
        vec2 pos2(100.0f + pos_y, 300);
        g_ptr<Text> twig = make<Text>(source_code,scene);
        twigs << twig;
        twig->initText(std::to_string(predicted),pos2);
    }
}

} 