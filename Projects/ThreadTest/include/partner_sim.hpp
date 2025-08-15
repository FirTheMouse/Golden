#pragma once

#include<extension/simulation.hpp>

namespace Golden
{
class P_Sim : virtual public Sim, public Singleton<P_Sim>
{
friend class Singleton<P_Sim>;
protected:
    P_Sim()
    {

    }
public:
    void runSlice() override {
       
    }
};
}