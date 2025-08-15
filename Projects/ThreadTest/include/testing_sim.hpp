#pragma once

#include<extension/simulation.hpp>

namespace Golden
{
class T_Sim : virtual public Sim, public Singleton<T_Sim>
{
friend class Singleton<T_Sim>;
protected:
    T_Sim()
    {

    }
public:
    void runSlice() override {
       
    }
};
}