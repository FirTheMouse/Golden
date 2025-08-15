#pragma once

#include<extension/simulation.hpp>
#include<test_object.hpp>

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
    std::vector<g_ptr<T_Object>> active;
    void runSlice() override {
        queueAndWait([this]{
        for(auto e : active)
        {
            ScriptContext ctx;
            e->run("onSlice",ctx);
        }
        });
    }
};
}