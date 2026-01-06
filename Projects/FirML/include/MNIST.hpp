#pragma once

#include<core.hpp>



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

    void run_mnist(g_ptr<Scene> scene,int epochs, int amt = -1) {
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
        print("Loaded ", train_imgs->data_.rows(), " images");

        auto w1 = weight(784, 128, 0.1f);
        auto b1 = bias(128);
        auto w2 = weight(128, 10, 0.1f);
        auto b2 = bias(10);

        list<Pass> network = {
            {MATMUL, w1}, {ADD_BIAS, b1}, {RELU},
            {MATMUL, w2}, {ADD_BIAS, b2}
        };
        list<g_ptr<tensor>> params = {w1, b1, w2, b2};

        Log::Line l;
        l.start();

        train_network(
            train_imgs, //Starting points
            train_labels, //Targets for the network
            network, //Nodes
            params, //Tensors
            SOFTMAX_CE, //Loss type
            epochs, //How many epochs to run 
            0.64f, //The learning rate
            64, //Batch size (if 0, turns off batching)
            1, //Logging interval (if 0, turns off logging)
            false, //Show final result?
            0.0f, //Disable grad clipping
            MEAN //Type of reduction to use
        );

        double time = l.end();
        print("Took ",time/1000000,"ms");

        // Run forward pass on test data
        auto test_output = test_imgs;
        for(auto p : network) {
            test_output = test_output->forward(p);
        }

        // Calculate accuracy
        int correct = 0;
        for(int i = 0; i < test_output->data_.rows(); i++) {
            // Find predicted class (max value in row)
            int pred_class = 0;
            float max_val = test_output->data_(i, 0);
            for(int j = 1; j < 10; j++) {
                if(test_output->data_(i, j) > max_val) {
                    max_val = test_output->data_(i, j);
                    pred_class = j;
                }
            }
            
            // Find true class (which column is 1 in one-hot encoding)
            int true_class = 0;
            for(int j = 0; j < 10; j++) {
                if(test_labels->data_(i, j) == 1.0f) {
                    true_class = j;
                    break;
                }
            }
            
            if(pred_class == true_class) correct++;
        }

        float accuracy = 100.0f * correct / test_output->data_.rows();
        print("Test Accuracy: ", accuracy, "%");

        list<int> indices = {randi(0, test_imgs->data_.rows() - 1), randi(0, test_imgs->data_.rows() - 1), randi(0, test_imgs->data_.rows() - 1)};

        for(int i = 0; i < 3; i++) {
            int idx = indices[i];
            
            // Display the image
            g_ptr<Quad> q = make<Quad>();
            scene->add(q);
            q->setTexture(mnist_to_texture(test_imgs, idx), 0);
            float pos_y = (float)i*300.0f;
            vec2 pos(pos_y,0.0f);
            q->setPosition(pos);  // Space them out
            vec2 scale(280, 280);
            q->scale(scale);

            // Get prediction
            auto single_img = test_imgs->get_batch(idx, 1);
            auto output = single_img;
            for(auto p : network) {
                output = output->forward(p);
            }
            
            // Find predicted class (max output)
            int predicted = 0;
            for(int j = 1; j < 10; j++) {
                if(output->data_(0, j) > output->data_(0, predicted)) {
                    predicted = j;
                }
            }
            
            // Display text showing prediction
            print("Image ", i, ": Predicted ", predicted);
            vec2 pos2(100.0f+pos_y,350);
            text::makeText(std::to_string(predicted),source_code,scene,pos2,1);
        }

    }

}




