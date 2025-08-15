#include <gui/quad.hpp>
#include <core/scriptable.hpp>


namespace Golden
{
    class G_Quad : virtual public Quad, virtual public Scriptable {
    public:
        using Quad::Quad;
        bool valid = true;
        float deadTime = 0.3f;
    };
}