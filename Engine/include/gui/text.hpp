#pragma once

#include<core/object.hpp>
#include<string>
#include<util/engine_util.hpp>
//#include<rendering/scene.hpp>
#include <util/d_list.hpp>


namespace Golden
{
using txt = g_ptr<d_list<size_t>>;
class Scene;
class Quad;

struct F_Glyph {
    vec2    size;       // width, height of glyph in pixels
    vec2    bearing;    // offset from baseline
    float   advance;    // how much to move forward (in pixels)
    vec2    uvPos;      // position in the atlas (0-1)
    vec2    uvSize;     // size in the atlas (0-1)
};

class Font : public Object {
public:
    Font() {}
    Font(const std::string& path,int size) {loadFromTTF(path,size);}
    map<char, F_Glyph> glyphs;
    uint atlasTexID = 0;  // OpenGL texture ID
    float lineHeight;

    void loadFromTTF(const std::string& ttfPath, int pxHeight);
};

struct T_Cursour {
T_Cursour() {}
T_Cursour(float _x,float _y,float _scale) : x(_x),y(_y),scale(_scale) {}
float x = 0.0f;
float y = 0.0f;
float scale = 1.0f;
};

namespace text {
g_ptr<Quad> makeChar(char c,g_ptr<Font> font,g_ptr<Scene> scene);
g_ptr<Quad> makeText(const std::string& text,g_ptr<Font> font,g_ptr<Scene> scene,vec2 start,float scale);

g_ptr<Quad> insertChar(size_t idx,char c,g_ptr<Quad> g);
g_ptr<Quad> insertText(size_t idx,const std::string text,g_ptr<Quad>);
g_ptr<Quad> removeChar(size_t idx,g_ptr<Quad> g);
g_ptr<Quad> removeText(size_t idx,size_t to,g_ptr<Quad> g);
g_ptr<Quad> setText(const std::string text,g_ptr<Quad> g);
g_ptr<Quad> setTextBefore(const std::string text,char delimiter,g_ptr<Quad> g,int nth=1);
g_ptr<Quad> setTextAfter(const std::string text,char delimiter,g_ptr<Quad> g,int nth=1);
std::string string_of(g_ptr<Quad> q);
g_ptr<Quad> char_at(size_t idx,g_ptr<Quad> g);
g_ptr<Quad> parent_of(g_ptr<Quad> g);
g_ptr<Quad> end_of(g_ptr<Quad> g);
vec2 center_of(g_ptr<Quad> g);

struct TextEditor {
    TextEditor(g_ptr<Scene> _scene) : scene(_scene) {}
    bool editing = false;
    g_ptr<Quad> crsr = nullptr;
    size_t crsPos = 0;
    float pause = 0.0f;
    float pause2 = 0.0f;
    float pause3 = 0.0f;
    float blink = 0.0f;
    bool blinkOn = true;
    int lastChar = 0;
    vec2 fromTo = vec2(1.0f,1.0f);
    std::string clipboard = "";
    g_ptr<Scene> scene;
    void tick(float tpf);
    void scan(g_ptr<Quad> g,bool need_click = false);
};
}

class Text : public Object {
    public:
        g_ptr<Scene> scene;
        std::string text;
        g_ptr<Font> font;
        float scale = 1.0f;
        float xCursor = 0.0f;
    
        void setText(const std::string& newText);
    };

}