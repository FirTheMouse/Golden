#pragma once

#include <util/util.hpp>


class Line {
    public:
        // explicit Line(std::string label) 
        //     : label_(std::move(label)), start_(std::chrono::steady_clock::now()) {}

        Line() {}
    
        ~Line() {
            // auto end = std::chrono::steady_clock::now();
            // auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start_).count();
            // std::cout << label_ << " : " << duration << " ns\n";
        }

    std::string label_;
    std::chrono::steady_clock::time_point start_;

    void start() {
        start_ = std::chrono::steady_clock::now();
    }

    double end() {
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start_).count();
        return (double)duration;
    }
    
    };



    