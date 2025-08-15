#pragma once

#include<rendering/scene.hpp>
#include<hel/hel_object.hpp>
#include<hel/hel_grid.hpp>

namespace Golden
{
class Hel : public Scene {
public:
    using Scene::Scene;

    map<str,list<g_ptr<H_Object>>> all;
    g_ptr<H_Grid> grid;

    void tickHel() {
        ScriptContext ctx;
        for(list<g_ptr<H_Object>> hl : all.getAll()) 
        {for(g_ptr<H_Object> h : hl){
            h->run("onTick",ctx);
        }}
    }

    g_ptr<H_Object> closestType(const vec3& to,const str& type)
    {
        float maxDist = 1000000;
        g_ptr<H_Object> closest;
        if(all.hasKey(type))
        {
            for(auto o : all.get(type))
            {
                float dist = o->getPosition().distance(to);
                if(dist<maxDist) 
                {maxDist = dist; closest = o;}
            }
        }
        return closest;
    }

    void add(const g_ptr<S_Object>& sobj) override
    {
        Scene::add(sobj);

        if(auto obj = g_dynamic_pointer_cast<H_Object>(sobj))
        {
            str dtype = obj->get<str>("dtype");
            if(!all.hasKey(dtype)) all.put(dtype,list<g_ptr<H_Object>>{});
            all.get(dtype) << obj;
            //obj->grid = grid.getPtr();
        }
        else if(auto _grid = g_dynamic_pointer_cast<H_Grid>(sobj))
        { grid = _grid; }

    }

    void removeObject(g_ptr<S_Object> robj) override
    {
        int ridx = -1;
        if(auto obj = g_dynamic_pointer_cast<H_Object>(robj))
        {
        list<g_ptr<H_Object>>& r = all.get(obj->get<str>("dtype"));
        for(int i=0;i<r.length();i++)
        {if(r[i]==obj) {ridx=i; break;}}
        if(ridx!=-1) r.removeAt(ridx);
        }

        robj->remove();
    }
};

}