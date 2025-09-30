#include <core/helper.hpp>
#include <util/util.hpp>
#include <util/meshBuilder.hpp>
#include <util/color.hpp>

using namespace Golden;

class SmartList : public Object {
public:
    SmartList(g_ptr<Scene> _scene,std::string _name) : scene(_scene), name(_name) {
        Script<> make_item("make_item",[_scene](ScriptContext& ctx){
            auto item = make<Quad>();
            int offset_x = 0;
            for(int i=0;i<_scene->quadActive.length();i++)
            {
                if(i>=_scene->quads.length()) break;
                if(!_scene->quadActive[i]) continue;
                auto g = _scene->quads[i];
                if(g->getCenter().y()>25) continue;
                offset_x+=50;
            }
            _scene->add(item);
            item->scale(vec2(50,50));
            item->setCenter(vec2(offset_x,25));
            item->set<int>("effort",0);
            item->set<list<size_t>>("depends",{});
            item->addScript("onUpdate",[_scene,item](ScriptContext& ctx){
                list<g_ptr<Quad>> depends;
                for(size_t i : item->get<list<size_t>>("depends"))
                {
                    if(auto g =_scene->getObject<Quad>(i))
                    {
                        if(g->get<list<size_t>>("depends").find(item->UUID)==-1)
                        {
                            //Circular dependency resolution
                        }
                        else 
                        {
                            depends << g;
                        }
                    }
                }
                if(depends.length()==0)
                {
                    item->flagOn("ready");
                    item->setColor(Color::GREEN);
                    if(item->getCenter().y()>25)
                        item->setCenter(item->getCenter().setY(25));
                }
                else 
                {
                    if(item->check("ready"))
                    {
                    item->flagOff("ready");
                    item->setColor(Color::RED);
                    }
                    
                    vec2 avg;
                    for(auto g : depends)
                    {
                        avg += g->getCenter();
                    }
                    avg = avg/depends.length();
                    item->setCenter(avg.addY(25));
                }
            });
            ctx.set<g_ptr<Object>>("toReturn",item);
        });
        _scene->define(_name+"_entry",make_item);
    }
    ~SmartList() {}
    g_ptr<Scene> scene;
    std::string name;

    q_map<std::string,g_ptr<Quad>> tasks;

    std::string suggestAction()
    {

    }

    void addTask(std::string name)
    {
        if(!tasks.hasKey(name))
        {
        auto g = scene->create<Quad>(name+"_entry");
        tasks.put(name,g);
        }
        else print("Task already exists!");
    }

    void removeTask(std::string name)
    {
        if(tasks.hasKey(name))
        {
        scene->recycle(tasks.get(name),name+"_entry");
        }
        else print("Task does not exist to be removed!");
    }

    g_ptr<Quad> getTask(std::string name)
    {
        if(tasks.hasKey(name))
        {
            return tasks.get(name);
        }
        else {
            print("Task does not exist!");
            return nullptr;
        }
    }

    void dependsOn(std::string task,std::string dependency)
    {
        if(auto t = getTask(task))
        {
            if(auto d = getTask(dependency))
            {
                t->get<list<size_t>>("depends").push(d);
            }
        }

    }
};

int main() {
    using namespace helper;

    Window window = Window(1280, 768, "attempt 1");
    auto scene = make<Scene>(window,2);
    Data d = make_config(scene,K);



    start::run(window,d,[&]{ 
        
    });

    return 0;
}
