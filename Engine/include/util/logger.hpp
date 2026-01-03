#pragma once

#include <util/util.hpp>
#include <util/engine_util.hpp>
#include <chrono>
#include <iostream>

namespace log {

// Provides the time it takes for a function to run, not avereged over iterations
double time_function(int ITERATIONS,std::function<void(int)> process) {
    auto start = std::chrono::high_resolution_clock::now();
    for(int i=0;i<ITERATIONS;i++) {
        process(i);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    return (double)time.count();
}


void run_rig(list<list<std::function<void(int)>>> f_table,list<list<std::string>> s_table,list<vec4> comps,bool warm_up,int PROCESS_ITERATIONS,int C_ITS) {
    list<list<double>> t_table;

    for(int c=0;c<f_table.length();c++) {
        t_table.push(list<double>{});
        for(int r=0;r<f_table[c].length();r++) {
            t_table[c].push(0.0);
        }
    }

    for(int m = 0;m<(warm_up?2:1);m++) {
        int C_ITERATIONS = m==0?warm_up?1:C_ITS:C_ITS;

        for(int c=0;c<t_table.length();c++) {
            for(int r=0;r<t_table[c].length();r++) {
                t_table[c][r]=0.0;
            }
        }

        for(int i = 0;i<C_ITERATIONS;i++)
        {
            for(int c=0;c<f_table.length();c++) {
                for(int r=0;r<f_table[c].length();r++) {
                    // if(r==0) print("Running: ",s_table[c][r]);
                    double time = time_function(PROCESS_ITERATIONS,f_table[c][r]);
                    t_table[c][r]+=time;
                }
            }
        }
        if(warm_up) {
        print("-------------------------");
        print(m==0 ? "      ==COLD==" : "       ==WARM==");
        }
        print("-------------------------");
        for(int c=0;c<t_table.length();c++) {
            for(int r=0;r<t_table[c].length();r++) {
                t_table[c][r]/=C_ITERATIONS;
                print(s_table[c][r],": ",t_table[c][r]," ns (",t_table[c][r] / PROCESS_ITERATIONS," ns per operation)");
            }
            print("-------------------------");
        }
        for(auto v : comps) {
            double factor = t_table[v.x()][v.y()]/t_table[v.z()][v.w()];
            std::string sfs;
            double tolerance = 5.0;
            if (std::abs(factor - 1.0) < tolerance/100.0) {
                sfs = "around the same as ";
            } else if (factor > 1.0) {
                double percentage = (factor - 1.0) * 100.0;
                sfs = std::to_string(percentage) + "% slower than ";
            } else {
                double percentage = (1.0/factor - 1.0) * 100.0;
                sfs = std::to_string(percentage) + "% faster than ";
            }
            print("Factor [",s_table[v.x()][v.y()],"/",s_table[v.z()][v.w()],
            "]: ",factor," (",s_table[v.x()][v.y()]," is ",sfs,s_table[v.z()][v.w()],")");
        }
        print("-------------------------");

    }
}

// A helper for using the benchmarking tools to reduce boilerplate
struct rig {
private:
    list<list<std::function<void(int)>>> f_table;
    list<list<std::string>> s_table;
    list<vec4> comps;
    map<std::string,ivec2> processes;
public:
    //Adds another table, tables are isolated test blocks
    void add_table() {
        f_table << list<std::function<void(int)>>{};
        s_table << list<std::string>{};
    }

    /// @brief Add a process to run
    /// @param process_name Name of the process, used for lookup and display when run
    /// @param process The function to time and run, the int argument is the process iteration, assuming PROCESS_ITERATIONS is not 1
    /// @param table Default value is 0, this can be used to split processes into distinct blocks when run
    void add_process(const std::string& process_name,std::function<void(int)> process,int table = 0) {
        while(f_table.length() <= table) add_table();
        processes.put(process_name,ivec2(table,f_table.get(table).length()));
        s_table.get(table) << process_name;
        f_table.get(table) << process;
    }

    /// @brief Add a comparison to be printed
    /// @param a Process to compare against
    /// @param b Process to compare to a
    void add_comparison(const std::string& a,const std::string& b) {
        try {
            ivec2 ap = processes.get(a);
            ivec2 bp = processes.get(b);
            comps << vec4(ap.x(),ap.y(),bp.x(),bp.y());
        } catch(std::exception e) {
            if(!processes.hasKey(a)) {
                print("rig::110 Unable to add comparison to rig: ",a," was never added as a process");
            }
            if(!processes.hasKey(b)) {
                print("rig::110 Unable to add comparison to rig: ",b," was never added as a process");
            }
        }
    }

    /// @brief Run the rig and print out the results of the benchmark
    /// @param C_ITS How many iterations of the processes there should be, this contributes to the averege
    /// @param warm_up Whether or not to do a cold run to warm up the cache
    /// @param PROCESS_ITERATIONS How many times each process should run, not part of the averege
    void run(int C_ITS,bool warm_up = false,int PROCESS_ITERATIONS = 1) {
        run_rig(f_table,s_table,comps,warm_up,PROCESS_ITERATIONS,C_ITS);
    }
};

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

}



    