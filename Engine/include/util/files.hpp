#pragma once

#include <iostream>
#include <filesystem>
#include <string>
#include <util/util.hpp>

namespace fs = std::filesystem;

list<std::string> list_subdirectories(const std::string& path,bool lastOnly = false) {
    list<std::string> toReturn;
    try {
        for (const auto& entry : fs::directory_iterator(path)) {
            if (entry.is_directory()) {
                std::string s = entry.path().string();
                if (lastOnly) {
                    s = entry.path().filename().string();
                }
                toReturn << s;
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << '\n';
    }
    return toReturn;
}

list<std::string> list_files(const std::string& path,bool lastOnly = false) {
    list<std::string> toReturn;
    try {
        for (const auto& entry : fs::directory_iterator(path)) {
            if (entry.is_regular_file()) {
                std::string s = entry.path().string();
                if (lastOnly) {
                    s = entry.path().filename().string();
                }
                toReturn << s;
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << '\n';
    }
    return toReturn;
}


