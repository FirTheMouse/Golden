#include<util/util.hpp>
#include<rendering/scene.hpp>
#include<core/helper.hpp>
#include<gui/text.hpp>
#include<util/color.hpp>


int main() {
    using namespace Golden;
    using namespace helper;

    Window window = Window(1280, 768, "Asteroid");
    auto scene = make<Scene>(window,2);
    Data d = make_config(scene,K);

    auto background = make<Quad>();
    scene->add(background);
    background->scale(vec2(4000,4000));
    background->setPosition(vec2(0,0));
    background->color = Color::BLACK;

    load_gui(scene,"Asteroid","astgui.fab");
    check_loaded({"spwn_ast","shp_clr"},scene);

    auto ship = make<Quad>(); scene->add(ship);
    ship->flagOn("ship");
    ship->scale(vec2(50,50));
    ship->setPosition(vec2(1230,718));
    ship->setColor(Color::BLUE);

    ship->addScript("onFire",[scene,ship](ScriptContext& ctx){
        auto proj = make<Quad>();
        proj->flagOn("proj");
        scene->add(proj);
        proj->scale(vec2(10,10));
        proj->setPosition(ship->getCenter().addY(ship->getScale().y()*-0.5f));
        proj->setLinearVelocity(vec2(0,-5));
        proj->setColor(Color::GREEN);

        proj->addScript("onUpdate",[scene,proj](ScriptContext& ctx){
            //Decomposing so we can modify individual aspects if need be
            //And becuase it makes it easier to downgrade the vec3
            vec3 v = proj->getVelocity().position;
            proj->move(vec2(v.x(),v.y()));
        });

        proj->addScript("onCollide",[scene,proj](ScriptContext& ctx){
            if(ctx.has("with"))
            {
                auto g = ctx.get<g_ptr<Quad>>("with");
                g->remove();
            }
            proj->remove();
        });
    });

    for(int i=0;i<13;i++)
    {
        auto ast = make<Quad>();
        scene->add(ast);
        auto s = randf(40.0f,100.0f);
        ast->scale(vec2(s,s));
        ast->setPosition(vec2(randf(0,2560),randf(0,1536)));
        ast->setColor(Color::GREY);
        ast->addScript("onCollide",[scene,ast](ScriptContext& ctx){
            if(ctx.has("with"))
            {
                auto g = ctx.get<g_ptr<Quad>>("with");
            g->remove();
            }
            ast->remove();
        });
    }


    start::run(window,d,[&]{
        ship->move(input_move_2d_keys(5));
        if(pressed(SPACE)) ship->run("onFire");


        if(scene->slotJustFired("spwn_ast"))
        {
            auto ast = make<Quad>();
            scene->add(ast);
            auto s = randf(40.0f,100.0f);
            ast->scale(vec2(s,s));
            ast->setPosition(vec2(randf(0,2560),randf(0,1536)));
            ast->setColor(Color::GREY);
            ast->addScript("onCollide",[scene,ast](ScriptContext& ctx){
                if(ctx.has("with"))
                {
                    auto g = ctx.get<g_ptr<Quad>>("with");
                    g->remove();
                }
                ast->remove();
            });
        }

        if(scene->slotJustFired("shp_clr"))
        {
            if(ship->color == Color::RED)
            ship->setColor(Color::BLUE);
            else if(ship->color == Color::BLUE)
            ship->setColor(Color::RED);
        }

        //Just doing a full scene run through 
        for(int i=0;i<scene->quadActive.length();i++)
        {
            //Early abort for multi-thread saftey
            if(i>=scene->quads.length()) break;
            if(!scene->quadActive[i]) continue;
            auto g = scene->quads[i];

            //Brute force naive collisons for now, because 2d doesn't have the neat physics 3d does at this point
            //This shows intenrals better though, and how the engine adapts to new cases
            for(int j=0;j<scene->quadActive.length();j++)
            {
                if(j>=scene->quadVelocities.length()) break;
                if(!scene->quadActive[j]) continue;
                auto q = scene->quads[j];
                if(g==q) continue;

                if((g->has("ship")||q->has("ship"))&&(g->has("proj")||q->has("proj"))) continue;

                if(g->pointInQuad(q->getCenter()))
                { 
                    ScriptContext ctx;
                    ctx.set<g_ptr<Quad>>("with",q);
                    g->run("onCollide");
                    ctx.set<g_ptr<Quad>>("with",g);
                    q->run("onCollide");
                    break;
                }
            }
        }
    });

    return 0;
}