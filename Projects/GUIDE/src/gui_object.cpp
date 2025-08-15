#include <guide/gui_object.hpp>
#include <util/meshBuilder.hpp>

namespace Golden
{
    
    Button::Button(const std::string& _name,float _size): Single() {
        name = _name;
        size = _size;
        setModel(Model(makeBox(size,size,0.2f,glm::vec4(1,0,0,1))));
    }
}