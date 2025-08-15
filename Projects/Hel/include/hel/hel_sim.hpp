#pragma once

#include<extension/simulation.hpp>
#include<hel/hel_scene.hpp>

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
        if(auto hel = g_dynamic_pointer_cast<Hel>(scene))
        {
           // hel->tickHel();
            queueAndWait([hel]{
                hel->tickHel();
            });
        }
    }
};
}