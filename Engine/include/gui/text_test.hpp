#pragma once

#include<core/object.hpp>
#include<rendering/scene.hpp>
#include<rendering/model.hpp>
#include<rendering/single.hpp>
#include<string>
#include <util/util.hpp>

namespace Golden
{

class TextChar : virtual public Single
{
public:
    using Single::Single;

    char c;
};

class TextBox
{
private:
    Model charModel(char c)
    {
        switch(c)
        {
            case '.': return Model("../models/text/dot.glb");
            case '0': return Model("../models/text/0.glb");
            case '1': return Model("../models/text/1.glb");
            case '2': return Model("../models/text/2.glb");
            case '3': return Model("../models/text/3.glb");
            case '4': return Model("../models/text/4.glb");
            case '5': return Model("../models/text/5.glb");
            case '6': return Model("../models/text/6.glb");
            case '7': return Model("../models/text/7.glb");
            case '8': return Model("../models/text/8.glb");
            case '9': return Model("../models/text/9.glb");
            default: return Model("../models/text/0.glb");
        }
    }
    g_ptr<Scene>  scene;
    vec3 pos = vec3(0,0,0);

public:
    TextBox(g_ptr<Scene> _scene) : scene(_scene) {}
    ~TextBox() {}

    std::vector<g_ptr<TextChar>> text;
    float scale = 3.3f;

    void setPosition(vec3 newPos) {
        for(auto c : text)
        {
            c->move(newPos);
        }
        pos = newPos;
    }

    void setText(list<char> newText)
    {
        if(text.size()>newText.length())
        {
            for(int i=0;i<(text.size()-newText.length());i++)
            {
                backspace();
            }
        }

        for(int i=0;i<newText.length();i++)
        {
            if(text.size()<i)
            {
                if(text[i]->c!=newText[i])
                {
                    text[i]->setModel(charModel(newText[i]));
                }
            }
            else
            {
                auto t = make<TextChar>(charModel(newText[i]));
                text.push_back(t);
                scene->add(t);
                t->setPosition(pos+vec3(i*scale,0,0));
            }
        }
    }

    void backspace()
    {
        text.pop_back();
    }

};


}