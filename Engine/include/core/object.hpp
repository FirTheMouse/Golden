#pragma once

#include <iostream>
#include <atomic>
#include <util/util.hpp>
#include<util/q_object.hpp>
#include<util/q_map.hpp>
#include<util/q_any.hpp>


namespace Golden {

class Type;


// class q_data : public q_object {
//     public:
//         q_data() {}
    
//         q_map<std::string,q_any> notes;
//         template<typename T = std::string>
//         void add(const std::string& label,T info)
//         {
//             notes.put(label,q_any(info));
//         }
    
//         template<typename T = std::string>
//         T get(const std::string& label)
//         {
//             if(!has(label)) std::cerr << "Data does not have label " << label <<"\n";
//             return q_any_cast<T>(notes.get(label));
//         }
    
//         bool has(const std::string& label)
//         {
//             return notes.hasKey(label);
//         }
    
//         bool check(const std::string& label)
//         {
//             if(!has(label)) return false;
//             try {
//                 return get<bool>(label);
//             }
//             catch(std::exception e)
//             {
//                 print("data::check::59 Attempted to check a non-bool in data");
//                 //Or just return false?
//                 return false;
//             }
//         }
    
//         bool toggle(const std::string& label) {
//             if(!has(label)) set<bool>(label,true);
//             bool toReturn = !get<bool>(label);
//             set<bool>(label,toReturn);
//             return toReturn;
//         }
    
//         void flagOn(const std::string& label) {set<bool>(label,true);}
//         void flagOff(const std::string& label) {set<bool>(label,false);}
    
//         template<typename T>
//         void set(const std::string& label,T info) {
//             if(!notes.set(label,info)) {
//                 add<T>(label,info);
//             }
//         }
    
//         template<typename T = int>
//         T inc(const std::string& label,T by)
//         {
//             if(has(label)) {set<T>(label,get<T>(label)+by);}
//             else {add<T>(label,by);}
//             return get<T>(label);
//         }
    
//         void debugData() {
//             notes.debugMap();
//         }
        
//         /// @brief Scans through based on provided list, returns all missing labels
//         list<std::string> validate(list<std::string> toCheck)
//         {
//             list<std::string> toReturn;
//             for(auto s : toCheck) if(!has(s)) toReturn << s;
//             return toReturn;
//         }
    
// };

struct ScriptContext
{
    Data data;

    ScriptContext() {

    }
    ~ScriptContext() {

    }

    template<typename T>
    T get(const std::string& label) { return data.get<T>(label); }

    template<typename T>
    void set(const std::string& label, T value) { data.set(label, value); }

    bool has (const std::string& label) {return data.has(label);}

    /// @brief  starts false, switches on each call
    bool toggle(const std::string& label) {return data.toggle(label);}
    bool check(const std::string& label) {return data.check(label);}
    void flagOn(const std::string& label) {data.flagOn(label);}
    void flagOff(const std::string& label) {data.flagOff(label);}

    template<typename T = int>
    T inc(const std::string& label,T by)
    { return data.inc<T>(label,by); }

};

template<typename func = std::function<void(ScriptContext&)>>
struct Script{
    std::string name;
    func run;
    Script() = default;
    Script (std::string _name,func&& behaviour) :
      name(_name), run(behaviour) {}
    // Copy constructor
    Script(const Script& other) = default;
    // Move constructor
    Script(Script&& other) noexcept = default;
    // Copy assignment operator
    Script& operator=(const Script& other) = default;
    // Move assignment operator
    Script& operator=(Script&& other) noexcept = default;
    ~Script() = default;

};

class Object : virtual public q_object {    
    public:
        Data data;
        std::string dtype = "";
        std::string debug_trace_path = "";
        int UUID;
        int ID;
        std::atomic<bool> recycled{false};

        template<typename T = std::string>
        T get(const std::string& label)
        { return data.get<T>(label); }

        template<typename T = std::string>
        void add(const std::string& label,T info)
        { data.add<T>(label,info); }

        template<typename T = std::string>
        void set(const std::string& label, T info)
        { data.set<T>(label,info); }

        bool has(const std::string& label)
        { return data.has(label); }

        /// @brief  starts false, switches on each call
        bool toggle(const std::string& label) {return data.toggle(label);}
        bool check(const std::string& label) {return data.check(label);}
        void flagOn(const std::string& label) {data.flagOn(label);}
        void flagOff(const std::string& label) {data.flagOff(label);}

        template<typename T = int>
        T inc(const std::string& label,T by)
        { return data.inc<T>(label,by); }

        q_map<std::string,Script<>> scripts;

        template<typename F>
        void addScript(const std::string& name, F&& f) {
            scripts.put(name,Script<>(name, std::function<void(ScriptContext&)>(std::forward<F>(f))));
        }

        void addScript(const Script<>& script) {
            scripts.put(script.name,script);
        }

        template<typename F>
        void replaceScript(const std::string& name, F&& f)
        {
            removeScript(name);
            addScript(name,f);
        }

        void removeScript(const std::string& name) {
            scripts.remove(name);
        }

        //void runAll() {for (const Script<>& s : scripts) {if(s.run) s.run();}}

        void run(const std::string& type,ScriptContext& ctx) {
            for(auto s : scripts.getAll(type)) s.run(ctx);
        }

        /// @brief Convenience for running when a context is not needed to reduce boilerplate
        void run(const std::string& type) {
            ScriptContext ctx;
            for(auto s : scripts.getAll(type)) s.run(ctx);
        }
    
        Type* type_ = nullptr;

        Object() {

        }
        virtual ~Object() {}

        Object(Object&& other) noexcept 
        : q_object(std::move(other)),
          data(std::move(other.data)),
          dtype(std::move(other.dtype)) {
            // scripts = other.scripts;
        }

        Object& operator=(Object&& other) noexcept {
            if (this != &other) {
                q_object::operator=(std::move(other));
                data = std::move(other.data);
                dtype = std::move(other.dtype);
                // scripts = std::move(other.scripts);
            }
            return *this;
        }
    };
}


