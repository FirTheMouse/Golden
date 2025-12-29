#include <extension/old_grid.hpp>

namespace Golden {

 void Cell::add(C_Object* object) {
    if (object) {objects.push_back(object); object->cells.push_back(index); types.push_back(object->dtype); solid=true;}
    }


void Cell::remove(C_Object* object) {
    objects.erase(std::remove_if(objects.begin(), objects.end(),
        [object](C_Object* o) {
            //o->type==object->type&&o->id==object->id
            return o == object;
        }), objects.end());
    std::string oType = object->dtype;
    int typeC = 0;
    int totalC = 0;
    for(auto o : objects)
    {
        totalC++;
        if(o->dtype==oType) typeC++;
    }
    if(totalC==0) solid=false;
    if(typeC==0) 
    types.erase(
        std::remove_if(types.begin(), types.end(),
        [oType](std::string s) {return s==oType;})
        ,types.end());
}

bool Cell::hasType(std::string type) { 
    for(auto o : objects) {
        //cout << objects.size() << endl;
       //cout << o->type << endl;
    if(o->dtype==type) return true;
    }
    return false;
}

std::vector<C_Object*> Cell::getType(std::string type) {   
    std::vector<C_Object*> toReturn;
    for(auto o : objects)
    if(o->dtype==type) {toReturn.push_back(o);}

    return toReturn;
}

void Grid::scanCells(vec3 center, float radius) {
        static thread_local std::unique_ptr<uint8_t[]> checked;
        static thread_local bool initialized = false;

        int gridSize = cellMap * cellMap;

        if (!initialized) {
            checked = std::make_unique<uint8_t[]>(gridSize);
            std::fill(checked.get(), checked.get() + gridSize, 0);
            initialized = true;
        }

        // Signal to clear the cache
        if (radius < 0.0f) {
            std::fill(checked.get(), checked.get() + gridSize, 0);
            return;
        }

        int gridRadius = static_cast<int>(radius / cellSize + 0.5f);
        g_ptr<Cell>  centerCell = getCell(center);
        int minZ = centerCell->z - gridRadius;
        int maxZ = centerCell->z + gridRadius;
        int minX = centerCell->x - gridRadius;
        int maxX = centerCell->x + gridRadius;

        for (int z = minZ; z <= maxZ; ++z) {
            for (int x = minX; x <= maxX; ++x) {
                int idx = cellIndex(x, z);
                if (!checked[idx]) {
                    //   for(std::string s : cells[idx].types)
                    //     cout << s << endl;
                    Grid::checkCell(idx);
                    checked[idx] = 1;
                }
            }
        }
    }

}


