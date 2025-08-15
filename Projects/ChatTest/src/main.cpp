#include<util/util.hpp>
#include<rendering/scene.hpp>
#include<core/helper.hpp>
#include<gui/text.hpp>
#include<util/color.hpp>

namespace Golden {


class ImagineThisIsJustScene {
public:
    q_list<std::string> msgQ;
    q_list<std::string> msgs;
    size_t max_buffer = 10;

    g_ptr<Scene> me;

    void sendMsg(std::string msg)
    {
        msgQ.push(msg);
    }
};

}

int main() {
    using namespace Golden;
    using namespace helper;

    Window window = Window(1280, 768, "testing");
    auto scene = make<Scene>(window,2);
    Data d = make_config(scene,K);

    auto background = make<Quad>();
    scene->add(background);
    background->scale(vec2(4000,4000));
    background->setPosition(vec2(0,0));
    background->color = Color::BLACK;

    load_gui(scene,"ChatTest","ctest.fab");
    check_loaded({"main","send_message","type_box"},scene);

    //Just imagine these are in Scene for now
    q_list<std::string> msgQ;

    q_list<std::string> msgs;


    size_t max_buffer = 10;

    auto chat =  scene->getSlot("main")[0];

    start::run(window,d,[&]{

      

      if(scene->slotJustFired("send_message"))
      {
        if(msgs.length()>=max_buffer)
        {
            std::string ref_txt = text::string_of(chat);
            msgs.remove(0);
            //Replace this with embeding the messages onto the line returns or sentinal chracters so we're not using line return only,
            //allowing users to place line returns in their message safely, if I come back to this seriously
            size_t end_line = ref_txt.find('\n', ref_txt.length()-1);
            text::removeText(end_line+1,ref_txt.length(),chat);
            text::insertText(end_line+2,msgQ.get(msgQ.length()-1,"ChatTest::run::59"),chat);
            msgQ.remove(msgQ.length()-1);
        }
      }
    });

    return 0;
}


int main() {
    using namespace Golden;
    using namespace helper;

    Window window = Window(1280, 768, "testing");
    auto scene = make<Scene>(window,2);
    Data d = make_config(scene,K);

    auto background = make<Quad>();
    scene->add(background);
    background->scale(vec2(4000,4000));
    background->setPosition(vec2(0,0));
    background->color = Color::BLACK;

    load_gui(scene,"ChatTest","ctest.fab");
    check_loaded({"main","send_message","type_box"},scene);

    // Chat system
    q_list<std::string> msgQ;  // Incoming messages
    q_list<std::string> msgs;  // Displayed messages
    size_t max_buffer = 10;

    auto chat = scene->getSlot("main")[0];
   // auto type_box = scene->getSlot("type_box")[0];

    // Initialize with some content
    text::setText("Chat started...\n", chat);

    // Simulate multiple users with threads
    std::atomic<bool> running{true};
    
    // Thread 1: Simulated user Alice
    std::thread alice([&]() {
        int counter = 0;
        while(running.load()) {
            std::this_thread::sleep_for(std::chrono::seconds(3));
            msgQ.push("Alice: Hello " + std::to_string(counter++));
        }
    });

    // Thread 2: Simulated user Bob  
    std::thread bob([&]() {
        int counter = 0;
        while(running.load()) {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            msgQ.push("Bob: How's it going? " + std::to_string(counter++));
        }
    });

    start::run(window,d,[&]{
        // Process incoming messages from queue
        if(msgQ.length() > 0) {
            std::string newMsg = msgQ.get(0, "ChatTest::processQueue");
            msgQ.remove(0);
            
            // Add to display buffer
            msgs.push(newMsg);
            
            // Update visual display
            std::string currentText = text::string_of(chat);
            text::setText(currentText + newMsg + "\n", chat);
            
            // Handle buffer overflow
            if(msgs.length() > max_buffer) {
                msgs.remove(0);
                // Remove oldest line from display
                std::string ref_txt = text::string_of(chat);
                size_t first_newline = ref_txt.find('\n');
                if(first_newline != std::string::npos) {
                    text::removeText(0, first_newline + 1, chat);
                }
            }
        }

        // Handle manual input (if send_message button is clicked)
        if(scene->slotJustFired("send_message")) {
            g_ptr<Quad> type_box = scene->getSlot("type_box")[0];
            std::string userInput = text::string_of(type_box);
            if(!userInput.empty()) {
                msgQ.push("You: " + userInput);
                text::setText("", type_box); // Clear input box
            }
        }
    });

    // Cleanup
    running.store(false);
    alice.join();
    bob.join();

    return 0;
}
int main() {
    using namespace Golden;

    // Chat system without visuals
    q_list<std::string> msgQ;
    q_list<std::string> msgs;
    size_t max_buffer = 1000;
    
    std::atomic<bool> running{true};
    std::atomic<size_t> messages_sent{0};
    std::atomic<size_t> messages_processed{0};

    print("starting");

    // Stress test: Multiple producer threads with tiny delays
    std::vector<std::thread> producers;
    for(int i = 0; i < 4; i++) { // Reduced to 4 threads
        producers.emplace_back([&, i]() {
            while(running.load()) {
                msgQ.push("Producer" + std::to_string(i) + ": Message " + std::to_string(messages_sent.fetch_add(1)));
                // Tiny delay to avoid nanosecond hammering
                std::this_thread::sleep_for(std::chrono::microseconds(1)); // 1 microsecond
            }
        });
    }

    // Consumer thread
    std::thread consumer([&]() {
        while(running.load() || msgQ.length() > 0) {
            if(msgQ.length() > 0) {
                std::string msg = msgQ.get(0, "PerfTest::consumer");
                msgQ.remove(0);
                
                msgs.push(msg);
                messages_processed.fetch_add(1);
                
                if(msgs.length() > max_buffer) {
                    msgs.remove(0);
                }
            }
            std::this_thread::yield();
        }
    });

    // Run for 10 seconds
    auto start_time = std::chrono::high_resolution_clock::now();
    std::this_thread::sleep_for(std::chrono::seconds(10));
    
    running.store(false);
    
    for(auto& producer : producers) {
        producer.join();
    }
    consumer.join();
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    print("Performance Test Results:");
    print("Duration: ", duration.count(), "ms");
    print("Messages sent: ", messages_sent.load());
    print("Messages processed: ", messages_processed.load());
    print("Throughput: ", (messages_processed.load() * 1000) / duration.count(), " messages/second");
    print("Final queue length: ", msgQ.length());

    return 0;
}