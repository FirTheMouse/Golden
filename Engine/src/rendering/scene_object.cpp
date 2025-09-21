#include <rendering/scene_object.hpp>
#include <rendering/scene.hpp>

namespace Golden {

    void S_Object::remove()
    {
        
    }

    void S_Object::saveBinary(const std::string& path) const {
        std::ofstream out(path, std::ios::binary);
        if (!out) throw std::runtime_error("Can't write to file: " + path);
        saveBinary(out);
        out.close();
    }

    void S_Object::loadBinary(const std::string& path) {
        std::ifstream in(path, std::ios::binary);
        if (!in) throw std::runtime_error("Can't read from file: " + path);
        loadBinary(in);
        in.close();
    }

    void S_Object::saveBinary(std::ostream& out) const {
        const char magic[] = "SOBJ";
        out.write(magic, 4);
    }

    void S_Object::loadBinary(std::istream& in) {
        char magic[4];
        in.read(magic, 4);
        if (std::strncmp(magic, "SOBJ", 4) != 0)
            throw std::runtime_error("Invalid file format");
    }

}