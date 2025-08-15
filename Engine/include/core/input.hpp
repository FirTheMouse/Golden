#pragma once
#include <GLFW/glfw3.h>
#include <util/util.hpp>
#include <core/singleton.hpp>

typedef enum {
    // Letters (ASCII)
    A = 65, B = 66, C = 67, D = 68, E = 69, F = 70, G = 71,
    H = 72, I = 73, J = 74, K = 75, L = 76, M = 77, N = 78,
    O = 79, P = 80, Q = 81, R = 82, S = 83, T = 84, U = 85,
    V = 86, W = 87, X = 88, Y = 89, Z = 90,

    // Numbers (ASCII)
    NUM_0 = 48, NUM_1 = 49, NUM_2 = 50, NUM_3 = 51, NUM_4 = 52,
    NUM_5 = 53, NUM_6 = 54, NUM_7 = 55, NUM_8 = 56, NUM_9 = 57,

    // Function keys (GLFW key codes)
    F1 = 290, F2 = 291, F3 = 292, F4 = 293, F5 = 294, F6 = 295,
    F7 = 296, F8 = 297, F9 = 298, F10 = 299, F11 = 300, F12 = 301,

    // Arrows (GLFW key codes)
    LEFT = 263, RIGHT = 262, UP = 265, DOWN = 264,

    // Space, Enter, Escape, Tab, Shift, Ctrl (GLFW key codes)
    SPACE = 32, ENTER = 257, ESCAPE = 256, TAB = 258, BACKSPACE = 259,
    LSHIFT = 340, LCTRL = 341, LALT = 342, CMD = 343,
    RSHIFT = 344, RCTRL = 345, RALT = 346,

    // Mouse buttons (if you want, GLFW codes)
    MOUSE_LEFT = 0, MOUSE_RIGHT = 1, MOUSE_MIDDLE = 2
} KeyCode;

inline bool keycodeToChar(int key, bool shift, char& out)
{
    /* ---------- 1. A-Z block ------------------------------------------ */
    if (key >= 'A' && key <= 'Z')
    {
        out = static_cast<char>( key + (shift ? 0 : 32) );  // +32 → lower case
        return true;
    }

    /* ---------- 2. Main digit block (top row, NOT num-pad) ------------ */
    if (key >= '0' && key <= '9')
    {
        static const char shiftedDigit[] = { ')', '!', '@', '#', '$',
                                             '%', '^', '&', '*', '(' };
        out = shift ? shiftedDigit[key - '0']       // ! @ # … )
                    : static_cast<char>(key);       // 0-9
        return true;
    }

    /* ---------- 3. Hard-wired punctuation keys ------------------------ */
    struct Pair { int glfw; char normal; char shifted; };

    static const Pair table[] = {
        /* key            unshift  shift */
        { GLFW_KEY_APOSTROPHE , '\'', '"'  },
        { GLFW_KEY_COMMA      , ',' , '<'  },
        { GLFW_KEY_MINUS      , '-' , '_'  },
        { GLFW_KEY_PERIOD     , '.' , '>'  },
        { GLFW_KEY_SLASH      , '/' , '?'  },
        { GLFW_KEY_SEMICOLON  , ';' , ':'  },
        { GLFW_KEY_EQUAL      , '=' , '+'  },
        { GLFW_KEY_LEFT_BRACKET , '[' , '{'},
        { GLFW_KEY_RIGHT_BRACKET, ']' , '}'},
        { GLFW_KEY_BACKSLASH    , '\\', '|' },
        { GLFW_KEY_GRAVE_ACCENT , '`' , '~' }
    };

    for (const auto& p : table)
        if (key == p.glfw) {
            out = shift ? p.shifted : p.normal;
            return true;
        }

    /* ---------- 4. whitespace / control ------------------------------ */
    if (key == GLFW_KEY_SPACE)  { out = ' ';  return true; }
    if (key == GLFW_KEY_ENTER)  { out = '\n'; return true; }
    if (key == GLFW_KEY_TAB)    { out = '\t'; return true; }

    return false;   // unmapped key
}


class Input : virtual public Golden::Singleton<Input> {
friend class Golden::Singleton<Input>; 
private:
    bool keys[364];
    bool prev[364];  
    double scrollX = 0.0, scrollY = 0.0;
protected:
    Input() {
        std::fill(std::begin(keys), std::end(keys), false);
        std::fill(std::begin(prev), std::end(prev), false);
    }
private:
    double mouseX = 0.0, mouseY = 0.0;
    double lastX = 0.0, lastY = 0.0;
    bool firstMouse = true;
public:
    void setKey(int key, bool pressed) {keys[key] = pressed;}
    bool keyPressed(int key) const { return keys[key]; }

    void advanceFrame() {
        std::memcpy(prev, keys, sizeof(keys));
    }

    bool keyJustPressed(int key) const {
        return keys[key] && !prev[key];
    }

    list<int> justPressed() const {
        list<int> out;
        for (int k = 0; k < 364; ++k)
            if (keys[k] && !prev[k])   out.push(k);
        return out;
    }

    list<int> pressed() const {
        list<int> toReturn;
        for(int i=0;i<364;i++)
        {
            if(keys[i]) {toReturn.push(i);}
        }
        return toReturn;
    }
    void resetKeyStates() {
        for(int i = 0; i < 256; i++) {
            keys[i] = false;
        }
    }

    void updateMouse(double xpos, double ypos) {
        if (firstMouse) {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }
        mouseX = xpos;
        mouseY = ypos;
    }

    double getMouseDeltaX() const { return mouseX - lastX; }
    double getMouseDeltaY() const { return lastY - mouseY; } // Y reversed
    void syncMouse() { lastX = mouseX; lastY = mouseY; }

    void resetMouseTracking() { firstMouse = true; }

    void decayScroll()
    {
        if(scrollY>0.0)
        {
            scrollY-=0.005;
            if(scrollY<0.03) scrollY=0;
        }
        if(scrollY<0.0)
        {
             scrollY+=0.005;
              if(scrollY>-0.03) scrollY=0;
        }

        if(scrollX>0.0)
        {
            scrollX-=0.005;
            if(scrollX<0.03) scrollX=0;
        }
        if(scrollX<0.0)
        {
             scrollX+=0.005;
              if(scrollX>-0.03) scrollX=0;
        }
    }

    void setScroll(double x, double y) { scrollX = x; scrollY = y; }
    double getScrollY() const { return scrollY; }
    double getScrollX() const { return scrollX; }
};