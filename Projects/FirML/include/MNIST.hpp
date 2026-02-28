#pragma once

#ifndef CRUMB_ROWS
#define CRUMB_ROWS 1
#define CRUMB_COLS 36
#endif

const int IMG = (0 << 16) | 1;

#include<util/cog.hpp>
#include<util/ml_core.hpp>
#include<gui/twig.hpp>

// Utility to read big-endian integers
int32_t read_int(std::ifstream& file) {
    unsigned char bytes[4];
    file.read((char*)bytes, 4);
    return (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];
}

namespace Eigen {

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

struct img_mat {
    float mat[28][28];
};

list<img_mat> load_mnist_as_matrix(const std::string& image_file, int max_samples = -1) {
    std::ifstream img_stream(image_file, std::ios::binary);
    if(!img_stream) throw std::runtime_error("Can't open " + image_file);

    int magic      = read_int(img_stream);
    int num_images = read_int(img_stream);
    int rows       = read_int(img_stream);
    int cols       = read_int(img_stream);

    if(max_samples > 0) num_images = std::min(num_images, max_samples);
    list<img_mat> crumbs;
    for(int i = 0; i < num_images; i++) {
        img_mat img;
        for(int r = 0; r < rows; r++) {
            for(int c = 0; c < cols; c++) {
                unsigned char pixel;
                img_stream.read((char*)&pixel, 1);
                img.mat[r][c] = pixel / 255.0f;
            }
        }
        crumbs << img;
    }
    return crumbs;
}

list<int> load_mnist_labels(const std::string& label_file, int max_samples = -1) {
    std::ifstream lbl_stream(label_file, std::ios::binary);
    if(!lbl_stream) throw std::runtime_error("Can't open " + label_file);

    int magic      = read_int(lbl_stream);
    int num_labels = read_int(lbl_stream);

    if(max_samples > 0) num_labels = std::min(num_labels, max_samples);

    list<int> labels;
    for(int i = 0; i < num_labels; i++) {
        unsigned char label;
        lbl_stream.read((char*)&label, 1);
        labels << (int)label;
    }
    return labels;
}

unsigned int crumb_to_texture(g_ptr<Crumb> crumb) {
    int w = CRUMB_COLS, h = CRUMB_ROWS;
    unsigned char* data = new unsigned char[w * h * 4];

    for(int r = 0; r < h; r++) {
        for(int c = 0; c < w; c++) {
            int flipped_r = (h - 1) - r;
            unsigned char pixel = (unsigned char)(crumb->mat[flipped_r][c] * 255.0f);
            int idx = (r * w + c) * 4;
            data[idx + 0] = pixel;
            data[idx + 1] = pixel;
            data[idx + 2] = pixel;
            data[idx + 3] = 255;
        }
    }

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    delete[] data;
    return tex;
}

list<g_ptr<Crumb>> gather_states_from_image(img_mat& img) {
    list<g_ptr<Crumb>> to_return;
    
    const int CELL_SIZE = 4;
    const int CELLS = 28 / CELL_SIZE;
    const int BINS = 9;
    const float BIN_SIZE = 180.0f / BINS;
    const float EPSILON = 0.001f;
    
    float cells[7][7][9] = {};
    
    for(int cy = 0; cy < CELLS; cy++) {
        for(int cx = 0; cx < CELLS; cx++) {
            for(int py = 0; py < CELL_SIZE; py++) {
                for(int px = 0; px < CELL_SIZE; px++) {
                    int x = cx * CELL_SIZE + px;
                    int y = cy * CELL_SIZE + py;
                    
                    float dx = (x > 0 && x < 27) ?
                        img.mat[y][x+1] - img.mat[y][x-1] : 0.0f;
                    float dy = (y > 0 && y < 27) ?
                        img.mat[y+1][x] - img.mat[y-1][x] : 0.0f;
                    
                    float mag = sqrt(dx*dx + dy*dy);
                    if(mag < 0.001f) continue;
                    
                    float angle = atan2(dy, dx) * 180.0f / M_PI;
                    if(angle < 0) angle += 180.0f;
                    
                    int bin = (int)(angle / BIN_SIZE) % BINS;
                    cells[cy][cx][bin] += mag;
                }
            }
        }
    }
    
    for(int by = 0; by < CELLS - 1; by++) {
        for(int bx = 0; bx < CELLS - 1; bx++) {
            float block[36] = {};
            int idx = 0;
            for(int cy = by; cy < by + 2; cy++)
                for(int cx = bx; cx < bx + 2; cx++)
                    for(int b = 0; b < BINS; b++)
                        block[idx++] = cells[cy][cx][b];
            
            float norm = EPSILON;
            for(int i = 0; i < 36; i++)
                norm += block[i] * block[i];
            norm = sqrt(norm);
            
            g_ptr<Crumb> crumb = clone(ZERO);
            for(int i = 0; i < 36; i++)
                crumb->mat[0][i] = block[i] / norm;
            
            to_return << crumb;
        }
    }
    
    return to_return;
}

void run_cog_mnist(g_ptr<Scene> scene, int amt = -1) {
    auto source_code = make<Font>(root()+"/Engine/assets/fonts/source_code_black.ttf",100);

    list<int> train_labels = load_mnist_labels(
        root()+"/Projects/FirML/assets/images/train-labels-idx1-ubyte", 
        amt
    );
    list<img_mat> train_imgs = load_mnist_as_matrix(
        root()+"/Projects/FirML/assets/images/train-images-idx3-ubyte", 
        amt
    );
    list<int> labels = load_mnist_labels(
        root()+"/Projects/FirML/assets/images/t10k-labels-idx1-ubyte",
        amt
    );
    list<img_mat> imgs = load_mnist_as_matrix(
        root()+"/Projects/FirML/assets/images/t10k-images-idx3-ubyte", 
        amt
    );

    // Sort both sets cyclically: 0,1,2,3,4,5,6,7,8,9,0,1,2...
    auto cyclic_sort = [](list<img_mat>& images, list<int>& lables) {
        list<list<int>> by_label(10);
        for(int i = 0; i < images.length(); i++)
            by_label[lables[i]] << i;

        list<img_mat> sorted_imgs;
        list<int> sorted_labels;
        int max_per_label = 0;
        for(int d = 0; d < 10; d++)
            if(by_label[d].length() > max_per_label)
                max_per_label = by_label[d].length();

        for(int i = 0; i < max_per_label; i++)
            for(int d = 0; d < 10; d++)
                if(i < by_label[d].length()) {
                    sorted_imgs << images[by_label[d][i]];
                    sorted_labels << lables[by_label[d][i]];
                }

        images = sorted_imgs;
        lables = sorted_labels;
    };

    cyclic_sort(train_imgs, train_labels);
    cyclic_sort(imgs, labels);

    print("Loaded ", train_imgs.length(), " training images");

    g_ptr<Cog> cog = make<Cog>(scene);
    cog->span = make<Span>();

    float train_match = 0.98f;
    float train_lr = 0.1f;
    int train_con_threshold = 50;

    float sleep_match = 0.9966f;
    float sleep_lr = 0.1f;

    for(int n=0;n<1;n++) {

        if(n==0) {
            train_match = 0.50f;
            train_lr = -1.0f;
            train_con_threshold = 100;
        
            sleep_match = 0.985f;
            sleep_lr = -1.0f;
        }
        else if(n==1) {
            train_lr = 0.5f;
        } else if(n==2) {
            train_lr = 0.1f;
        }

        cog->newline("training"); //Start TRN

        for(int i = 1; i < train_imgs.length(); i++) {
            print("IMG: ",i," RECENT_EPISODES: ",cog->recent_episodes.length()," CONSOLIDATED: ",cog->consolidated_episodes.length());
            g_ptr<Episode> ep = cog->form_episode(
                gather_states_from_image(train_imgs[i-1]),
                gather_states_from_image(train_imgs[i]),
                train_labels[i-1],i);
            if(cog->consolidate_episode(ep, cog->recent_episodes, IMG, train_match, train_lr) > train_con_threshold) {
                cog->recent_episodes.erase(ep);
                cog->consolidate_episode(ep, cog->consolidated_episodes, IMG, sleep_match, sleep_lr);
                // cog->log("Consolidated epiosde: ",ep->to_string());
            } else if(cog->recent_episodes.length()>1000) {
                for(int e=cog->recent_episodes.length()-1;e>=0;e--) {
                    g_ptr<Episode> to_consolidate = cog->recent_episodes[e];
                    if(to_consolidate->hits<=10) {
                        cog->recent_episodes.removeAt(e);
                    }
                }
            }
        }
        print("Run ",n+1," time: ",ftime(cog->endline())); //End TRN
        // print("Lines:");
        // cog->span->print_all();
        // print("===========");
        int pre_sleep_len = cog->consolidated_episodes.length();
        // print("Sleeping");
        map<int,int> id_counts;
        for(int e=cog->consolidated_episodes.length()-1;e>=0;e--) {
            g_ptr<Episode> to_consolidate = cog->consolidated_episodes[e];
            id_counts.getOrPut(to_consolidate->action_id,0)+=1;
            cog->consolidated_episodes.removeAt(e);
            cog->consolidate_episode(to_consolidate,cog->consolidated_episodes, IMG, sleep_match, sleep_lr);
        }
        cog->span = make<Span>();
        // print("Consolidated epiosdes:");
        // for(auto c : cog->consolidated_episodes) {
        //     print(c->to_string());
        // }
        // for(auto e : id_counts.entrySet()) {
        //     print("Num: ",e.key," Pre-sleep consolidated: ",e.value);
        // }

        print("Classifying...");
        cog->newline("CLS"); //Start CLS
        int correct = 0;
        int total = imgs.length();
        int sequence_correct = 0;
        map<int,int> per_class_correct;
        map<int,int> per_class_total;
        int confusion[10][10] = {};  // confusion[predicted][actual]

        for(int i = 1; i < total; i++) {
            g_ptr<Episode> test_ep = cog->form_episode(
                gather_states_from_image(imgs[i-1]),
                gather_states_from_image(imgs[i]),
                labels[i-1],i);
            
            float best_score = -1.0f;
            int predicted = -1;
            
            g_ptr<Episode> best_proto = nullptr;
            for(auto proto : cog->consolidated_episodes) {
                float score = cog->match_episode(test_ep, IMG, proto, IMG);
                if(score > best_score) {
                    best_score = score;
                    predicted = proto->action_id;
                    best_proto = proto;
                }
            }

            list<g_ptr<Crumb>> predicted_next = test_ep->states;
            for(auto& c : predicted_next) c = clone(c); // make mutable
            cog->apply_delta(best_proto, predicted_next);

            // Then match predicted_next against consolidated episodes by state
            float best_seq_score = -1.0f;
            int predicted_next_label = -1;
            for(auto proto : cog->consolidated_episodes) {
                float score = cog->match_episode(proto, IMG, predicted_next, IMG);
                if(score > best_seq_score) {
                    best_seq_score = score;
                    predicted_next_label = proto->action_id;
                }
            }

            if(predicted_next_label == labels[i]) // labels[i] is the actual next digit
                sequence_correct++;
            
            int actual = labels[i-1];
            per_class_total.getOrPut(actual, 0) += 1;
            if(predicted == actual) {
                correct++;
                per_class_correct.getOrPut(actual, 0) += 1;
            }
            if(predicted >= 0 && predicted < 10)
                confusion[predicted][actual]++;
        }

        print("Classification Time: ", ftime(cog->endline()));
        print("Accuracy: ", (float)correct / total * 100.0f, "%");
        print("Correct: ", correct, " / ", total);
        for(int d = 0; d < 10; d++) {
            int c = per_class_correct.getOrPut(d, 0);
            int t = per_class_total.getOrPut(d, 0);
            print("Digit ", d, ": ", c, "/", t, " = ", (float)c/t*100.0f, "%");
        }

        print("\nConfusion Matrix (rows=predicted, cols=actual):");
        print("pred\\act  0    1    2    3    4    5    6    7    8    9");
        for(int p = 0; p < 10; p++) {
            std::string row = "   " + std::to_string(p) + "     ";
            for(int a = 0; a < 10; a++) {
                std::string val = std::to_string((int)((float)confusion[p][a] / per_class_total[a] * 100.0f));
                while(val.length() < 4) val = " " + val;
                row += val + " ";
            }
            print(row);
        }
        print("Training. match threshold: ",train_match," LR: ",train_lr," consolidate threshold: ",train_con_threshold);
        print("Sleep. match threshold: ",sleep_match," LR: ",sleep_lr);
        print("left over after sleep: ",cog->consolidated_episodes.length()," from: ",pre_sleep_len," recent: ",cog->recent_episodes.length());
        print("Accuracy: ", (float)correct / total * 100.0f, "%");
        print("Seq Accuracy: ", (float)sequence_correct / total * 100.0f, "%");

        // for(auto c : cog->consolidated_episodes) {
        //     if(c->action_id==1) {
        //         print(c->to_string());
        //     }
        // }

        cog->consolidated_episodes.clear();
        cog->recent_episodes.clear();
    }

}