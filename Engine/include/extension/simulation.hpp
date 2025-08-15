#pragma once

#include <chrono>
#include <iostream>
#include <string>
#include <cmath> 
#include <thread>
#include <atomic>
#include <mutex>
#include <deque>
#include <thread>
#include <future>
#include <functional>
#include <util/util.hpp>
#include <core/input.hpp>
#include <core/scriptable.hpp>
#include <rendering/scene.hpp>
#include <core/object.hpp>
#include <core/singleton.hpp>


class Processor
{

};

namespace Golden    
{

class Sim 
{
public:
std::atomic<float> tps{0.3f};
std::chrono::steady_clock::time_point lst = std::chrono::steady_clock::now();
bool logSPS = false;

protected:
Sim()
{

}
std::atomic<bool> runningSlice;
std::thread simulationThread;
std::atomic<bool> shouldStopThread;
std::mutex sceneMutex;
float sliceTime = 0;
std::atomic<float> sliceSpeed{0.3f};

std::mutex taskQueueMutex;
std::deque<std::function<void()>> taskQueue;
private:


void simulationLoop() {
auto lastSliceTime = std::chrono::steady_clock::now();
auto SPSOutput = std::chrono::steady_clock::now();
    int sliceCounter = 0;
    while (!shouldStopThread) {
        auto currentTime = std::chrono::steady_clock::now();
        float delta = std::chrono::duration<float>(currentTime - lastSliceTime).count();
        if (runningTurn && !runningSlice && delta >= sliceSpeed) {
            runningSlice = true;
            runSlice();
            slice++;
            runningSlice = false;
            float fallback = sliceSpeed;
            tps = tps>sliceSpeed ? fallback : delta;
            sliceCounter++;
            lastSliceTime = currentTime;
            lst = currentTime;
        }
        else if(!runningTurn) tps = 0.0f;

        if(std::chrono::duration<float>(currentTime - SPSOutput).count()>=1.0f)
        {
            if(logSPS)
            {
            std::cout << sliceCounter << " SPS " << std::endl;
            }
            sliceCounter=0;
            SPSOutput = currentTime;
        }
        
        // Don't burn CPU waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

virtual void runSlice() = 0;

// void runSlice()
// {
//     runningSlice = true;
//     {
//         std::lock_guard<std::mutex> lock(sceneMutex);
//         if (scene) {
//             scene->tickEnvironment(slice+1100);
//         }
//     }
    
//     {
//         // std::lock_guard<std::mutex> lockEntity(entityMutex);
//         // for(const auto& e : active)
//         // {
//         //     if(e->ticked)
//         //     {
//         //         //e->runAll();
//         //         e->ticked = false;
//         //         queueMainThreadCall([e](){e->ticked=true;});
//         //     }
//         // }
//         // // queueMainThreadCall([this](){for(auto c : scene->grid.cellsAroundUltraFast(vec3(0,0,0),40.0f))
//         // // {scene->grid.updateCellColorFast(*c,glm::vec4(0,0,0,1));} 
//         // // });
//     }
//     slice++;
//     runningSlice = false;
// }

public:
g_ptr<Scene> scene;
void setScene(g_ptr<Scene> _scene) {
    std::lock_guard<std::mutex> lock(sceneMutex);
    scene = _scene;}

std::atomic<int> slice;
std::atomic<bool> runningTurn;

void startSimulation() {
    shouldStopThread = false;
    simulationThread = std::thread(&Sim::simulationLoop, this);
}

void stopSimulation() {
    shouldStopThread = true;
    if (simulationThread.joinable()) {
        simulationThread.join();
    }
}
    ~Sim() {
    stopSimulation();
}

// template<typename T>
// void addActive(std::g_ptr<T> ent)
// {
//     static_assert(std::is_base_of<Scriptable, T>::value, "T must be a derived scriptable");
//     std::lock_guard<std::mutex> lock(entityMutex);
//     active.push_back(ent);
// }

// template<typename T>
// void removeActive(std::g_ptr<T>  ent)
// {
//     std::lock_guard<std::mutex> lock(entityMutex);
//     auto it = std::remove_if(active.begin(), active.end(),
//         [ent](const std::shared_ptr<Scriptable>& e) {
//             return e == ent;
//         });

//     if (it != active.end()) {
//         active.erase(it, active.end());
//     }
// }

void setSpeed(float speed)
{
    if(speed<=0.0f) {runningTurn = !runningTurn;}
    else {sliceSpeed = speed;}
}

void queueTask(std::function<void()> func) {
std::lock_guard<std::mutex> lock(taskQueueMutex);
taskQueue.push_back(func);
}

void queueAndWait(std::function<void()> func) {
std::promise<void> done;
auto future = done.get_future();

queueTask([&done, func]() {
    func(); 
    done.set_value();
});

future.get(); // Wait until it's done
}

void flushTasks() {
std::deque<std::function<void()>> localQueue;
{
    std::lock_guard<std::mutex> lock(taskQueueMutex);
    std::swap(localQueue, taskQueue);
}

auto startTime = std::chrono::steady_clock::now();
while (!localQueue.empty() &&
    std::chrono::duration<float>(std::chrono::steady_clock::now() - startTime).count() < 0.01f) 
{
    auto task = localQueue.front();
    localQueue.pop_front(); 

    task();
}
if (!localQueue.empty()) std::cerr << "[WARN] Sim task flush incomplete!" << std::endl;

// Push any unprocessed tasks back into the main queue
{
    std::lock_guard<std::mutex> lock(taskQueueMutex);
    while (!localQueue.empty()) {
        taskQueue.push_back(std::move(localQueue.front()));
        localQueue.pop_front();
    }
}
}
};
}