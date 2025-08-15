#pragma once

#include <chrono>
#include <iostream>
#include <string>
#include <cmath> 
#include <thread>
#include <atomic>
#include <mutex>
#include <deque>
#include <future>
#include <functional>
#include <util/util.hpp>
#include <core/input.hpp>
#include <core/scriptable.hpp>
#include <extension/transform_controller.hpp>
#include <duel/duelManager.hpp>
#include <extension/simulation.hpp>

namespace Golden
{

class GDM
{
public:
    std::atomic<float> tps{0.3f};
    std::chrono::steady_clock::time_point lst = std::chrono::steady_clock::now();
private:
    GDM()
    {

    }
    GDM(const GDM&) = delete;
    GDM& operator=(const GDM&) = delete;


    std::atomic<bool> runningSlice;
    std::thread simulationThread;
    std::atomic<bool> shouldStopThread;
    std::mutex duelMutex;
    float sliceTime = 0;
    std::atomic<float> sliceSpeed{0.3f};

    std::mutex mainThreadQueueMutex;
    std::deque<std::function<void()>> mainThreadQueue;


    void simulationLoop() {
    auto lastSliceTime = std::chrono::steady_clock::now();
    auto SPSOutput = std::chrono::steady_clock::now();
        int sliceCounter = 0;
        while (!shouldStopThread) {
            auto currentTime = std::chrono::steady_clock::now();
            float delta = std::chrono::duration<float>(currentTime - lastSliceTime).count();
            if (runningTurn && !runningSlice && delta >= sliceSpeed) {
                runSlice();
                float fallback = sliceSpeed;
                tps = tps>sliceSpeed ? fallback : delta;
                sliceCounter++;
                lastSliceTime = currentTime;
                lst = currentTime;
            }
            else if(!runningTurn) tps = 0.0f;

            if(std::chrono::duration<float>(currentTime - SPSOutput).count()>=1.0f)
            {
                //std::cout << sliceCounter << " SPS " << std::endl;
                sliceCounter=0;
                SPSOutput = currentTime;
            }
            
            // Don't burn CPU waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    void queueAndWaitMainThreadCall(std::function<void()> func) {
        std::promise<void> done;
        auto future = done.get_future();
    
        queueMainThreadCall([&done, func]() {
            func(); 
            done.set_value();
        });
    
        future.get(); // Wait until it's done
    }
    

    //Run slice through proccesors, if we have extra time left in the slice, precompute for the next slice
    void runSlice()
    {
        runningSlice = true;
        {
            std::lock_guard<std::mutex> lock(duelMutex);
            if (duel) {
                queueAndWaitMainThreadCall([this]{
                    duel->tickSet();
                });
            }
        }
        slice++;
        runningSlice = false;
    }

public:
    static GDM& get()
    {
        static GDM instance;
        return instance;
    }

    static T_Controller& get_T()
    {
        return T_Controller::get();
    }
    g_ptr<Duel> duel;
    void setDuel(g_ptr<Duel> _duel) {
        std::lock_guard<std::mutex> lock(duelMutex);
        duel = _duel;
    }

    std::atomic<int> slice;
    std::atomic<bool> runningTurn;
    bool deBounceSpace = false;

    void startSimulation() {
        shouldStopThread = false;
        simulationThread = std::thread(&GDM::simulationLoop, this);
    }
    
    void stopSimulation() {
        shouldStopThread = true;
        if (simulationThread.joinable()) {
            simulationThread.join();
        }
    }
     ~GDM() {
        stopSimulation();
    }

    void tick(float tpf) {
        if(Input::get().keyPressed(SPACE)&&!deBounceSpace) {runningTurn = !runningTurn; deBounceSpace=true;}
        if(!Input::get().keyPressed(SPACE)&&deBounceSpace) {deBounceSpace = false;}
        if(Input::get().keyPressed(NUM_1)) sliceSpeed = 0.3f;
        if(Input::get().keyPressed(NUM_2)) sliceSpeed = 0.1f;
        if(Input::get().keyPressed(NUM_3)) sliceSpeed = 0.03f;
        if(Input::get().keyPressed(NUM_4)) sliceSpeed = 0.016f;
        if(Input::get().keyPressed(NUM_5)) sliceSpeed = 0.001f;
    }

void queueMainThreadCall(std::function<void()> func) {
    std::lock_guard<std::mutex> lock(mainThreadQueueMutex);
    mainThreadQueue.push_back(func);
}

void flushMainThreadCalls() {
    std::deque<std::function<void()>> localQueue;
    {
        std::lock_guard<std::mutex> lock(mainThreadQueueMutex);
        std::swap(localQueue, mainThreadQueue);
    }

    auto startTime = std::chrono::steady_clock::now();
    while (!localQueue.empty() &&
        std::chrono::duration<float>(std::chrono::steady_clock::now() - startTime).count() < 0.01f) 
    {
        // if(std::chrono::duration<float>(std::chrono::steady_clock::now() - startTime).count() >= 0.005f)
        // cout << "failed flush" << endl;

        auto task = localQueue.front();
        localQueue.pop_front(); 

        task();
    }
    //cout << localQueue.size() << endl;

    if(!localQueue.empty()) std::cout << "Failed flush" << std::endl;

    // Push any unprocessed tasks back into the main queue
    {
        std::lock_guard<std::mutex> lock(mainThreadQueueMutex);
        while (!localQueue.empty()) {
            mainThreadQueue.push_back(std::move(localQueue.front()));
            localQueue.pop_front();
        }
    }
}
};

class T_Sim : virtual public Sim, public Singleton<T_Sim>
{
friend class Singleton<T_Sim>;
protected:
    T_Sim()
    {

    }
public:
    void runSlice() override {
        {
            if(scene)
            {
            std::lock_guard<std::mutex> lock(sceneMutex);
            if(auto duel = g_dynamic_pointer_cast<Duel>(scene))
            {
                queueAndWait([duel]{
                    duel->tickSet();
                });
            }
            }
        }
    }
};

}