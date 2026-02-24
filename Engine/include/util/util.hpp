#pragma once

#include <any>
#include <stdlib.h>

#include <initializer_list>
#include <fstream>
#include <sstream>
#include <string>

#include <util/list.hpp>
#include <util/map.hpp>
#include<util/basic.hpp>

inline list<std::string> split_str(const std::string& s,char delimiter)
{
    list<std::string> toReturn;
    int last = 0;
    for(int i=0;i<s.length();i++)
    {
        if(s.at(i)==delimiter) {
            toReturn << s.substr(last,i-last);
            last = i+1;
        }
    }
    if(last<s.length())
    {
        toReturn << s.substr(last,s.length()-last);
    }
    return toReturn;
}


inline std::string wrap_str(const std::string& s,const std::string& c) {
    return c+s+c;
}

inline std::string trim_str(const std::string& s,const char c) {
    if (s.size() >= 2 && s.front() == c && s.back() == c)
        return s.substr(1, s.size() - 2);
    return s; 
}



class Data{
public:
    Data() {}

    map<std::string,std::any> notes;

    template<typename T = std::string>
    void add(const std::string& label,T info)
    {
        notes.put(label,std::any(info));
    }

    template<typename T = std::string>
    T get(const std::string& label)
    {
        #if !DISABLE_BOUNDS_CHECK
            if(!has(label)) std::cerr << "Data does not have label " << label <<"\n";
        #endif
        return std::any_cast<T>(notes.get(label));
    }

    bool has(const std::string& label)
    {
        return notes.hasKey(label);
    }

    bool check(const std::string& label)
    {
        if(!has(label)) return false;
        try {
            return get<bool>(label);
        }
        catch(std::exception e)
        {
            print("data::check::59 Attempted to check a non-bool in data");
            //Or just return false?
            return false;
        }
    }

    bool toggle(const std::string& label) {
        if(!has(label)) set<bool>(label,true);
        bool toReturn = !get<bool>(label);
        set<bool>(label,toReturn);
        return toReturn;
    }

    void flagOn(const std::string& label) {set<bool>(label,true);}
    void flagOff(const std::string& label) {set<bool>(label,false);}

    template<typename T>
    void set(const std::string& label,T info) {
        if(!notes.set(label,info))
            add<T>(label,info);
    }

    template<typename T = int>
    T inc(const std::string& label,T by)
    {
        if(has(label)) {set<T>(label,get<T>(label)+by);}
        else {add<T>(label,by);}
        return get<T>(label);
    }

    void debugData() {
        notes.debugMap();
    }
    
    /// @brief Scans through based on provided list, returns all missing labels
    list<std::string> validate(list<std::string> toCheck)
    {
        list<std::string> toReturn;
        for(auto s : toCheck) if(!has(s)) toReturn << s;
        return toReturn;
    }

    // void print() const
    // {
        
    // }

};

static inline std::string green(const std::string& text) {
    return "\x1b[32m"+text+"\x1b[0m";
}
static inline std::string yellow(const std::string& text) {
    return "\x1b[33m"+text+"\x1b[0m";
}
static inline std::string red(const std::string& text) {
    return "\x1b[31m"+text+"\x1b[0m";
}

static std::string ftime(double t) 
{
    if(t >= 100000000) {
        return red(std::to_string(t/1000000000.0)+"s");
    } else if(t >= 100000) {
        return yellow(std::to_string(t/1000000.0)+"ms");
    } else { 
        return  green(std::to_string(t/1000.0)+"ns");
    } 
}

namespace Log {
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
        double total_time_ = 0.0;

        void start() {
            start_ = std::chrono::steady_clock::now();
        }

        double end() {
            auto end = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start_).count();
            total_time_ += duration;
            return (double)duration;
        }
        
    };
}

inline std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) throw std::runtime_error("Could not open file: " + filename);
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

inline void writeFile(const std::string& filename, const std::string& contents) {
    std::ofstream file(filename, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!file) throw std::runtime_error("Could not open file for writing: " + filename);

    file.write(contents.data(), static_cast<std::streamsize>(contents.size()));
    if (!file) throw std::runtime_error("Failed while writing file: " + filename);
}

inline void editTextFile(  const std::string& filename, const std::function<void(std::string&)>& editor) {
    std::string text = readFile(filename);
    editor(text);
    writeFile(filename, text);
}


