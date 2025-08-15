#pragma once
#include <iostream>
#include <core/object.hpp>
#include <util/util.hpp>
#include <functional>
#include <vector>


namespace Golden {


// struct ScriptContext
// {
//     Data data;

//     template<typename T>
//     T get(const std::string& label) { return data.get<T>(label); }

//     template<typename T>
//     void set(const std::string& label, T value) { data.set(label, value); }

//     bool has (const std::string& label) {return data.has(label);}

//     /// @brief  starts false, switches on each call
//     bool toggle(const std::string& label) {return data.toggle(label);}
//     bool check(const std::string& label) {return data.check(label);}
//     void flagOn(const std::string& label) {data.flagOn(label);}
//     void flagOff(const std::string& label) {data.flagOff(label);}

//     template<typename T = int>
//     T inc(const std::string& label,T by)
//     { return data.inc<T>(label,by); }

// };

// template<typename func = std::function<void(ScriptContext&)>>
// struct Script{
//     std::string name;
//     func run;
//     Script() = default;
//     Script (std::string _name,func&& behaviour) :
//       name(_name), run(behaviour) {}
//     // Copy constructor
//     Script(const Script& other) = default;
//     // Move constructor
//     Script(Script&& other) noexcept = default;
//     // Copy assignment operator
//     Script& operator=(const Script& other) = default;
//     // Move assignment operator
//     Script& operator=(Script&& other) noexcept = default;
//     ~Script() = default;

// };

//Might need to add deconstructer for the scripts list, not sure! 
//Add this as an ovveride to cleanup or in the deconstructer itself 
//Which might just call cleanup
class Scriptable : virtual public Object {
public:
  Scriptable() {}
  ~Scriptable() = default;

};

}