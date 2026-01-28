#pragma once

#include<rendering/scene.hpp>
#include<util/logger.hpp>


#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

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
    unsigned int atlasTexID = 0;  // OpenGL texture ID
    float lineHeight;
    g_ptr<Geom> geom;
    std::string name = "bullets";

    void loadFromTTF(const std::string& ttfPath, int pxHeight) {
        set<std::string>("path",ttfPath);
        set<float>("scale",pxHeight);

        size_t slash_pos = ttfPath.find_last_of('/')+1;
        name = ttfPath.substr(slash_pos,ttfPath.find_last_of('.')-slash_pos);

        // 1. Read the whole TTF file
        std::vector<uint8_t> ttf;
        {
            std::ifstream in(ttfPath, std::ios::binary);
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

        geom = make<Geom>();
        geom->texture = atlasTexID;
    }
};


class Text : public Object {
public:
    g_ptr<Font> font = nullptr;
    g_ptr<Scene> scene = nullptr;
    list<g_ptr<Quad>> chars;

    Text(g_ptr<Font> _font, g_ptr<Scene> _scene) : scene(_scene), font(_font) {}

    g_ptr<Quad> makeChar(char c)
    {
        //Pooling is begging for an overhaul too... those strings are stinking up the codebase and ScriptContext is so ready to be axed
        std::string pool_name = font->name+"_char";
         if(!scene->pools.hasKey(pool_name)) {
            Script<> make_char("make_char",[&](ScriptContext& ctx){
                auto q = make<Quad>(font->geom);
                scene->add(q);
                q->setPhysicsState(P_State::NONE);
                ctx.set<g_ptr<Object>>("toReturn",q);
            });
            scene->define(pool_name,make_char);
        }
        auto q = scene->create<Quad>(pool_name);
        q->opt_char = c;
        //Cleaning up just in case, because we do recycle through pools
        q->opt_x_offset = 0;
        q->opt_offset = vec2(0,0);
        q->opt_delta = vec2(0,0);
        q->parent = nullptr;
        q->isAnchor = false;
        q->opt_float = 0;
        q->opt_float_2 = 0;
        q->children.clear();
        q->parents.clear();

        q->joint = [q](){
            g_ptr<Quad> anchorParent = q->opt_ptr;
            //This has no handeling for if we are the anchor, that's because anchors don't get jointed currently
            //but when we want to attatch text to non-text quads, we'll need to handle this!
            if(!anchorParent || !anchorParent->isAnchor) {
                anchorParent = q->parent;
                while(anchorParent && !anchorParent->isAnchor) {
                    if(anchorParent->parent) {
                        anchorParent = anchorParent->parent;
                    } else {
                        anchorParent->isAnchor = true;
                        break;
                    }
                }
                q->opt_ptr = anchorParent;
            }
            
            vec2 anchorScaleFactor = anchorParent->scaleVec / anchorParent->opt_delta;
            float anchorRotation = anchorParent->rotation;
            vec2 anchorPos = anchorParent->position;
            
            float totalAdvance = 0;
            float totalLineHeight = 0;
            float lineHeight = 50.0f; //ADD: get from font
            if(q->opt_char=='\n') {
                totalAdvance = -anchorParent->opt_x_offset;
                totalLineHeight = q->parent->opt_float_2+lineHeight;
            } else {
                totalAdvance = q->parent->opt_float+q->opt_x_offset;
                totalLineHeight = q->parent->opt_float_2;
            }
            //SPECIAL HANDELING ALERT! This cursour behaviour should probably be provided via a flag in the future.
            if(q->opt_char=='|') {
                q->opt_float = q->parent->opt_float;
            } else {
                q->opt_float = totalAdvance;
            }
            q->opt_float_2 = totalLineHeight;

            
            totalAdvance *= anchorScaleFactor.x();
            totalLineHeight *= anchorScaleFactor.y();
            
            float yOffset = (q->opt_offset.y() - anchorParent->opt_offset.y()) * anchorScaleFactor.y() + totalLineHeight;
            vec2 localOffset = vec2(totalAdvance, yOffset);

            
            // Rotate around anchor
            float cos_r = std::cos(anchorRotation);
            float sin_r = std::sin(anchorRotation);
            vec2 rotatedOffset;

            rotatedOffset = vec2(
                localOffset.x() * cos_r - localOffset.y() * sin_r,
                localOffset.x() * sin_r + localOffset.y() * cos_r
            );
            
            q->position = anchorPos + rotatedOffset;
            q->scaleVec = q->opt_delta * anchorScaleFactor;
            q->rotation = anchorRotation;
            
            return true;
        };

        if (font->glyphs.hasKey(c)) {
            F_Glyph& g = font->glyphs.get(c);
            q->scale(g.size);
            q->setData(vec4(g.uvPos.x(), g.uvPos.y(), g.uvSize.x(), g.uvSize.y()));
    
            q->opt_x_offset = g.advance;
            q->opt_offset = g.bearing;
            q->opt_delta = g.size;
        } 

        return q;
    }

    list<g_ptr<Quad>> makeText(const std::string& content,vec2 pos = {0,0}) {
        for(char c : content) {
            auto q = makeChar(c);
            chars << q;
        }

        chars[0]->isAnchor = true;

        for(int i=1;i<chars.length();i++) {
            chars[i-1]->children << chars[i];
            chars[i]->parent = chars[i-1];
        }

        chars[0]->setPosition(pos);

        return chars;
    }

    g_ptr<Quad> insertChar(char c,g_ptr<Quad> at) {
        if(at==nullptr) return nullptr;
        auto q = makeChar(c);
        g_ptr<Quad> oldChild = at->children.empty() ? nullptr : at->children[0];
        q->parent = at;
        at->children.clear();
        at->children << q;
        
        if(oldChild) {
            q->children << oldChild;
            oldChild->parent = q;
        }

        chars.insert(q,chars.find(at)+1);
        chars[0]->updateTransform();

        return q;
    }

    g_ptr<Quad> insertChar(char c,int at) {
        return insertChar(c,chars[at]);
    }

    g_ptr<Quad> removeChar(g_ptr<Quad> at) {
        if(at==nullptr) return nullptr;
        g_ptr<Quad> parent = at->parent;

        // if(at->isAnchor) { 
        //     print("Trying to remove an anchor");
        //     if(!parent) print("No parent");
        //     else print("Parent");

        // }

        if(parent) {
            parent->children.erase(at);
            if(!at->children.empty()) {
                parent->children << at->children[0];
                at->children[0]->parent = parent;
                at->children.clear();
            }
        } 
        else {
            if(!at->children.empty()) {
                at->children[0]->parent = nullptr;
                at->children[0]->isAnchor = true;
                at->children[0]->setPosition(at->getPosition());
                at->children[0]->opt_float = 0;
                at->children[0]->opt_float_2 = 0;  
                at->children.clear();
            } else {
                //There's no children to make the new anchor, i.e, the text is empty.
                //This is repeated logic, it remains for now so-as to be explicit about the case.
                chars.erase(at);
                scene->recycle(at);
                return nullptr;
            }
        }

        chars.erase(at);
        scene->recycle(at);

        if(!chars.empty())
            chars[0]->updateTransform();
        return parent;
    }

    g_ptr<Quad> removeChar(int at) {
        return removeChar(chars[at]);
    }

    void removeText(int at,size_t ammount) {
        if(at==-1||chars.empty()) return;
        for(size_t i=0;i<ammount;i++)
        {
            removeChar(at);
        }
    }
    void removeText(g_ptr<Quad> at,size_t ammount) {
        removeText(chars.find(at),ammount);
    }

    void insertText(const std::string& content, int at,vec2 pos = {0,0}) {
        if(at==-1||chars.empty()) {
            makeText(content,pos);
        }
        else 
            for(size_t i=0;i<content.length();i++)
            {
                insertChar(content.at(i),at++);
            }
    }

    void insertText(const std::string& content, g_ptr<Quad> at,vec2 pos = {0,0}) {
        insertText(content,chars.find(at),pos);
    }

    void setText(const std::string& content) {
        vec2 backup_pos = {0,0};
        if(!chars.empty()) {
            backup_pos = chars[0]->getPosition();
            removeText(0,chars.length());
        }
        insertText(content,0,backup_pos);
    }

};