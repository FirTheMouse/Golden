#include<tensor.hpp>
#include<ml_util.hpp>
#include<MNIST.hpp>
#include<core.hpp>



using namespace Eigen;

#define ON_WINDOWS 0


int main() {

    int win_size_x = 450;
    int win_size_y = 200;

    #if ON_WINDOWS
        win_size_x*=2;
        win_size_y*=2;
    #endif

    Window window = Window(win_size_x, win_size_y, "FirML 0.0.2");
    auto scene = make<Scene>(window,2);
    scene->camera.toIso();
    scene->tickEnvironment(0);
    Data d = helper::make_config(scene,K);
    auto source_code = make<Font>(root()+"/Engine/assets/fonts/source_code_black.ttf",100);

    int amt = 1000;

    auto [train_imgs, train_labels] = load_mnist(
        root()+"/Projects/Learning/assets/images/train-images-idx3-ubyte", 
        root()+"/Projects/Learning/assets/images/train-labels-idx1-ubyte", 
        amt
    );
    auto [test_imgs, test_labels] = load_mnist(
        root()+"/Projects/Learning/assets/images/t10k-images-idx3-ubyte", 
        root()+"/Projects/Learning/assets/images/t10k-labels-idx1-ubyte", 
        amt
    );
    print("Loaded ", train_imgs->data_.rows(), " images");

    auto w1 = weight(784, 128, 0.1f);
    auto b1 = bias(128);
    auto w2 = weight(128, 10, 0.1f);
    auto b2 = bias(10);

    list<Pass> network = {
        {MATMUL, w1}, {ADD_BIAS, b1}, {RELU},
        {MATMUL, w2}, {ADD_BIAS, b2}, {SOFTMAX}
    };
    list<g_ptr<tensor>> params = {w1, b1, w2, b2};

    Log::Line l;
    l.start();

    train_network(
        train_imgs, //Starting points
        train_labels, //Targets for the network
        network, //Nodes
        params, //Tensors
        CROSS_ENTROPY, //Loss type
        10, //How many epochs to run 
        0.01f, //The learning rate
        64, //Batch size (if 0, turns off batching)
        1, //Logging interval (if 0, turns off logging)
        false //Show final result?
    );

    double time = l.end();
    print("Took ",time/1000000,"ms");

    // Run forward pass on test data
    auto test_output = test_imgs;
    for(auto p : network) {
        test_output = test_output->forward(p.op, p.param);
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
            output = output->forward(p.op, p.param);
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

    start::run(window,d,[&]{

    });

    return 0;
}




