#include <gui/text.hpp>
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#include<rendering/scene.hpp>
//#include <GLFW/glfw3.h>


//The performance for this text is absolutely horrendus, because we're using Data all over the place, this desperetly needs to be fixed in the future
namespace Golden
{
    void Font::loadFromTTF(const std::string& path, int pxHeight) {
        set<std::string>("path",path);
        set<float>("scale",pxHeight);
        // 1. Read the whole TTF file
        std::vector<uint8_t> ttf;
        {
            std::ifstream in(path, std::ios::binary);
            in.seekg(0, std::ios::end);
            size_t len = in.tellg();
            in.seekg(0);
            ttf.resize(len);
            in.read(reinterpret_cast<char*>(ttf.data()), len);
        }
    
        // 2. Init stb font
        stbtt_fontinfo info;
        if (!stbtt_InitFont(&info, ttf.data(), 0)) {
            throw std::runtime_error("Bad TTF");
        }
    
        // 3. Bake an atlas (simple 16×6 grid for 96 printable ASCII chars)
        const int firstChar = 32, charCount = 96;
        const int cell = pxHeight + 2;              // 1-pixel padding
        const int cols = 16;
        const int rows = (charCount + cols - 1) / cols;
        const int atlasW = cols * cell;
        const int atlasH = rows * cell;
    
        std::vector<uint8_t> atlas(atlasW * atlasH, 0);
    
        float scale = stbtt_ScaleForPixelHeight(&info, pxHeight);
        int ascent, descent, lineGap;
        stbtt_GetFontVMetrics(&info, &ascent, &descent, &lineGap);
        lineHeight = (ascent - descent) * scale;
    
        for (int ch = 0; ch < charCount; ++ch) {
            int c = firstChar + ch;
            int cx = (ch % cols) * cell;
            int cy = (ch / cols) * cell;
    
            int w, h, xoff, yoff;
            uint8_t* bmp = stbtt_GetCodepointBitmap(
                &info, 0, scale, c, &w, &h, &xoff, &yoff);
    
            // Stamp into atlas
            for (int y = 0; y < h; ++y) {
                int dstRow = cy + (h - 1 - y);  
                memcpy(&atlas[dstRow * atlasW + cx],
                    &bmp[y * w], w);
            }
            stbtt_FreeBitmap(bmp, nullptr);
    
            int adv, lsb;
            stbtt_GetCodepointHMetrics(&info, c, &adv, &lsb);
    
            F_Glyph g;
            g.size     = vec2(w, h);
            g.bearing  = vec2(xoff, yoff);
            g.advance  = adv * scale;
            g.uvPos    = vec2(float(cx) / atlasW, float(cy) / atlasH);
            g.uvSize = vec2(float(w) / atlasW, float(h) / atlasH);
           
            // float uvX = float(cx) / atlasW;
            // float uvY = float(cy + h) / atlasH;             // +h → go to the glyph’s baseline
            // uvY = 1.0f - uvY;                               // flip into GL space
            
            // g.uvPos  = vec2(uvX, uvY);
            // g.uvSize = vec2(float(w) / atlasW,
            //                 float(h) / atlasH);
            glyphs.put((char)c, g);
        }

    
        // 4. Upload atlas to OpenGL
        glGenTextures(1, &atlasTexID);
        glBindTexture(GL_TEXTURE_2D, atlasTexID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED,
                     atlasW, atlasH, 0, GL_RED, GL_UNSIGNED_BYTE, atlas.data());
        // swizzle so (R,R,R,R) is returned instead of (R,0,0,1)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_RED);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_RED);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_RED);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    void Text::setText(const std::string& newText) {
        text = newText;
    }
    namespace text {
    g_ptr<Quad> makeChar(char c,g_ptr<Font> font,g_ptr<Scene> scene)
    {
         //This is to intilize the pooling for text characters, it shouldn't really be here but I don't know where else to put it
         if(!scene->pools.hasKey("_text_char")) {
            Script<> make_char("make_char",[scene](ScriptContext& ctx){
                auto q = make<Quad>();
                scene->add(q);
                ctx.set<g_ptr<Object>>("toReturn",q);
            });
            scene->define("_text_char",make_char);
        }

        if(c=='\n')
        {
            auto nl = scene->create<Quad>("_text_char");
            nl->set<float>("advance", 0.f);
            nl->set<vec2>("bearing", vec2(0,0));
            nl->scale(vec2(0,0));  
            nl->setTexture(0,0);  
            nl->set<char>("char",c);
            nl->set<vec2>("size",vec2(0,0));
            nl->setColor(glm::vec4(0));
            return nl;
        }
        else if(c=='\"')
        {
            auto nl = scene->create<Quad>("_text_char");
            nl->set<float>("advance", 0.f);
            nl->set<vec2>("bearing", vec2(0,0));
            nl->scale(vec2(0,0));  
            nl->setTexture(0,0);  
            nl->set<char>("char",c);
            nl->set<vec2>("size",vec2(0,0));
            nl->setColor(glm::vec4(0));
            return nl;
        }
        
        if (!font->glyphs.hasKey(c)) return nullptr;
        F_Glyph& g = font->glyphs.get(c);
        if(c=='|') g.advance = 5.0f;

        auto q = scene->create<Quad>("_text_char");
        q->setTexture(font->atlasTexID,0);
        q->scale(g.size);
        q->uvRect = glm::vec4(g.uvPos.x(), g.uvPos.y(), g.uvSize.x(), g.uvSize.y());
        q->color = glm::vec4(1);
        q->set<char>("char",c);
        q->set<float>("advance",g.advance);
        q->set<vec2>("bearing",g.bearing);
        q->set<vec2>("size",g.size);
        q->set<g_ptr<Font>>("font",font);
        return q;
    }

    g_ptr<Quad> makeText(const std::string& text,g_ptr<Font> font,g_ptr<Scene> scene,vec2 start,float scale) {
        g_ptr<Quad> first;
        g_ptr<Quad> last;
        int pID = 0;
        auto chars = make<d_list<size_t>>();
        chars->set<float>("scale",scale);
        chars->set<g_ptr<Font>>("font",font);
        chars->set<g_ptr<Scene>>("scene",scene);
        std::string newText = "\""+text+"\"";
        chars->set<std::string>("string",text);
        for (char c : newText) {
            g_ptr<Quad> q = makeChar(c,font,scene);
            vec2 bearing = q->get<vec2>("bearing");
            q->setPosition(vec2(start.x() + (bearing.x()*scale),
                (bearing.y()*scale)+start.y()));   // baseline handling simplified
            q->scale(q->get<vec2>("size")*scale);
            q->set<size_t>("pID",pID);
            pID++;
            chars->push(q->UUID);
            if(!first) {first = q;}
            else {first->addChild(q,true);}

            float dx = q->get<float>("advance") * scale;
            float left = first->getPosition().x();
            if(c=='\n') {
                dx-=(start.x()-left);
                start.addY(50.0f*scale);}

            start.addX(dx);

            q->set<txt>("chars",chars);
            last = q;
        }
        chars->add<g_ptr<Quad>>("parent",first);
        chars->add<g_ptr<Quad>>("last",last);
        return first;
    }

    g_ptr<Quad> insertChar(size_t idx,char c,g_ptr<Quad> g)
    {
    
        auto chars = g->get<txt>("chars");
        if (!chars || idx > chars->length()) return g;

        g_ptr<Font> font = chars->Object::get<g_ptr<Font>>("font");
        g_ptr<Scene> scene = chars->Object::get<g_ptr<Scene>>("scene");
    
        auto at = scene->getObject<Quad>((*chars)[idx]);
        if (!at) return g;
        vec2 pos = at->getPosition();
    
        float scale =  chars->Object::get<float>("scale");

        g_ptr<Quad> q = makeChar(c,font,scene);
        vec2 brQ = q->get<vec2>("bearing");
        vec2 brA = at->get<vec2>("bearing");
        vec2 br = brQ-brA;
        q->setPosition(pos+(br*scale));   // baseline handling simplified
        //q->setPosition(pos);
        q->set<size_t>("pID",idx);
        q->set<txt>("chars", chars);
    
        float dx = q->get<float>("advance") * scale;
        float dy = 0.0f;
        float left = scene->getObject<Quad>(chars->operator[](0))->getPosition().x();

        if(c=='\n') {dy = 50.0f*scale; dx-=(pos.x()-left);}
    
        g_ptr<Quad> parent = chars->Object::get<g_ptr<Quad>>("parent");
        // if(!g->parent) parent = g;
        // else parent = g->parent;
        
        //parent->detatchAll();
        bool onLine = true;
        for (std::size_t i = idx; i < chars->length(); ++i)
        {
            g_ptr<Quad> quad = scene->getObject<Quad>((*chars)[i]);
            if(!quad) continue;
            // if(quad->get<char>("char")=='\n') {dy+=50.0f*scale; dx-=left;}
            if(quad->get<char>("char")=='\n') {onLine = false;}

            bool isParent = quad==parent;
            if(!isParent) quad->removeFromParent();
            quad->set<size_t>("pID", i+1);
            vec2 p = quad->getPosition();
            if(onLine) p.addX(dx);
            p.addY(dy);
            quad->setPosition(p);
            quad->scale(quad->get<vec2>("size")*scale);
            if(!isParent) parent->addChild(quad,true);
            
        }
        chars->insert(q->UUID,idx);
        q->scale(q->get<vec2>("size")*scale);
        parent->addChild(q,true);
        std::string str = chars->Object::get<std::string>("string");
        str.insert(idx-1, 1, c);
        chars->Object::set<std::string>("string", str);
        return q;
    }    

    g_ptr<Quad> insertText(size_t idx,const std::string text,g_ptr<Quad> g)
    {
        for(char c : text) {insertChar(idx++,c,g);}
        return g;
    }

    g_ptr<Quad> removeChar(size_t idx,g_ptr<Quad> g) {
        auto chars = g->get<txt>("chars");
        if (!chars || idx > chars->length()) return g;
    
        g_ptr<Scene> scene = chars->Object::get<g_ptr<Scene>>("scene");

        auto at = scene->getObject<Quad>((*chars)[idx]);
        if (!at) return g;


        float scale =  chars->Object::get<float>("scale");
        float dx = at->get<float>("advance") * scale;
        float dy = 0.0f;
        float left = scene->getObject<Quad>(chars->operator[](0))->getPosition().x();

        if(at->get<char>("char")=='\n') {dy = 50.0f*scale; dx-=(at->getPosition().x()-left);}

        g_ptr<Quad> parent = chars->Object::get<g_ptr<Quad>>("parent");
        // g_ptr<Quad> parent;
        // if(!g->parent) parent = g;
        // else parent = g->parent;

        if(at==parent) {at->detatchAll();}
        else {at->removeFromParent();}
        chars->removeAt(at->get<size_t>("pID"));
        scene->recycle(at,"_text_char");

        bool onLine = true;
        for (std::size_t i = idx; i < chars->length(); ++i)
        {
            g_ptr<Quad> quad = scene->getObject<Quad>((*chars)[i]);
            if(!quad) continue;
            if(quad->get<char>("char")=='\n') {onLine = false;}

            bool isParent = quad==parent;
            if(!isParent) quad->removeFromParent();
            quad->set<size_t>("pID", i);
            vec2 p = quad->getPosition();
            if(onLine) p.addX(-dx);
            p.addY(-dy);
            quad->setPosition(p);
            quad->scale(quad->get<vec2>("size")*scale);
            if(!isParent) parent->addChild(quad,true);
        }
    
        std::string str = chars->Object::get<std::string>("string");
        str.erase(idx-1,1);
        chars->Object::set<std::string>("string", str);
        return parent;
    }

    g_ptr<Quad> removeText(size_t idx,size_t to,g_ptr<Quad> g) {
        for(size_t i=0;i<to;i++)
        {
            removeChar(idx,g);
        }
        return g;
    }
    g_ptr<Quad> setText(const std::string text,g_ptr<Quad> g) {
        std::string s = string_of(g);
        removeText(1,s.length(),g);
        insertText(1,text,g);
        return g;
    }

    g_ptr<Quad> setTextBefore(const std::string text,char delimiter,g_ptr<Quad> g,int nth)
    {
        std::string string = text::string_of(g);
        int sLoc = 1;
        int pLoc = 1;
        /// @todo Add suppourt for nth delimiter, i.e x/y/z nth=2 means x/[here]/z for before
        for(int i=0;i<string.length();i++) {
            if(string.at(i)==delimiter) sLoc=i;
        }
        text::removeText(1,sLoc,g);
        text::insertText(1,text,g);
        return g;
    }

    g_ptr<Quad> setTextAfter(const std::string text,char delimiter,g_ptr<Quad> g,int nth)
    {
        std::string string = text::string_of(g);
        int sLoc = 1;
        int pLoc = 1;
        for(int i=0;i<string.length();i++) {
            if(string.at(i)==delimiter) sLoc=i;
        }
        text::removeText(sLoc+2,string.length()-(sLoc+1),g);
        text::insertText(sLoc+2,text,g);
        return g;
    }

    std::string string_of(g_ptr<Quad> q) {
        if(q->has("char")) 
            return q->get<txt>("chars")->Object::get<std::string>("string");
        else return "text::stringOf::311 NOT A TEXT QUAD";
    }

    g_ptr<Quad> char_at(size_t idx,g_ptr<Quad> g)
    {
        size_t uuid = g->get<txt>("chars")->list::get(idx);
        return g->get<txt>("chars")->Object::get<g_ptr<Scene>>("scene")->getObject<Quad>(uuid);
    }

    g_ptr<Quad> parent_of(g_ptr<Quad> g)
    {
        if(g->has("char")) 
            return g->get<txt>("chars")->Object::get<g_ptr<Quad>>("parent");
        else print("text::parentOf::330 NOT A TEXT QUAD"); 
        return g;
    }

    g_ptr<Quad> end_of(g_ptr<Quad> g)
    {
        if(g->has("char")) 
        {
            // auto chars = g->get<txt>("chars");
            // g_ptr<Scene> scene = chars->Object::get<g_ptr<Scene>>("scene");
            // return scene->getObject(g->get<txt>("chars")->last());
            return g->get<txt>("chars")->Object::get<g_ptr<Quad>>("last");
        }
        else print("text::endOf::365 NOT A TEXT QUAD"); 
        return g;
    }

    vec2 center_of(g_ptr<Quad> g) {
        auto chars = g->get<txt>("chars");
        g_ptr<Scene> scene = chars->Object::get<g_ptr<Scene>>("scene");
    
        vec2 minBounds(FLT_MAX, FLT_MAX);
        vec2 maxBounds(-FLT_MAX, -FLT_MAX);
        
        for(int i = 1; i < chars->length() - 1; i++) {
            auto charQuad = scene->getObject<Quad>((*chars)[i]);
            vec2 charPos = charQuad->getPosition();
            vec2 charScale = charQuad->getScale();
            
            vec2 charMin = charPos;
            vec2 charMax = charPos + charScale;
            
            minBounds = vec2(std::min(minBounds.x(), charMin.x()),
                            std::min(minBounds.y(), charMin.y()));
            maxBounds = vec2(std::max(maxBounds.x(), charMax.x()),
                            std::max(maxBounds.y(), charMax.y()));
        }
        
        return (minBounds + maxBounds) / 2.0f;
    }

    // auto chars = g->get<txt>("chars");
    // g_ptr<Scene> scene = chars->Object::get<g_ptr<Scene>>("scene");
    // float totalAdvance = 0;
    // float scale = chars->Object::get<float>("scale");
    
    // // Sum up all the advance widths
    // for(int i = 1; i < chars->length() - 1; i++) { // Skip quotes
    //     auto charQuad = scene->getObject((*chars)[i]);
    //     totalAdvance += charQuad->get<float>("advance") * scale;
    // }
    
    // vec3 startPos = scene->getObject((*chars)[1])->getPosition(); // First real char
    // return vec2(startPos.x() + totalAdvance/2.0f, startPos.y());
    

    }
}