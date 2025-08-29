#pragma once

#include <util/util.hpp>
#include<core/object.hpp>
#include<util/d_list.hpp>

namespace Golden
{

class NumGrid : public Object
{
public:
    float cellSize;
    int mapSize;
    int cellMap;
    int half;
    float y_level = 0;
    list<list<int>> cells;
    map<std::string,list<int>> typeCache;

    NumGrid(float cellSize_, int mapSize_) : cellSize(cellSize_), mapSize(mapSize_) { 
        cellMap = static_cast<int>(mapSize / cellSize);
        float mapArea = cellMap*cellMap;
        cells.reserve(mapArea);
        for(int i=0;i<mapArea;i++)
        {
            cells.push(list<int>());
        }
        half = cellMap/2;
    }

    list<int> getCell(const vec3& worldPos)
    {
    int gridX = std::round(worldPos.x() / cellSize);
    int gridZ = std::round(worldPos.z() / cellSize);
    return getCell(gridX, gridZ);
    }

    inline list<int> getCell(int gridX, int gridZ) {
        int index = cellIndex(gridX,gridZ);
        if(index>=cells.length()) {
        //print("Grid::52 implment cell addition in the future");
        return list<int>{};
        //return make<d_list<g_ptr<Object>>>();
        }
        return cells.get(index);
    }

    inline int cellIndex(int x, int z) const {
        return (z + half) * cellMap + (x + half);
    }

    int toIndex(const vec3& worldPos)
    {
    int gridX = std::round(worldPos.x() / cellSize);
    int gridZ = std::round(worldPos.z() / cellSize);
    return cellIndex(gridX,gridZ);
    }

    vec3 posToGrid(const vec3& worldPos)
    {
    int gridX = std::round(worldPos.x() / cellSize);
    int gridZ = std::round(worldPos.z() / cellSize);
    return vec3(gridX,0,gridZ);
    }

    vec3 snapToGrid(const vec3& worldPos) const {
        int gridX = std::round(worldPos.x() / cellSize);
        int gridZ = std::round(worldPos.z() / cellSize);
        return vec3(gridX * cellSize, y_level, gridZ * cellSize);
    }

    inline int cellDistance(int idxA, int idxB) const
    {
        int dx = std::abs((idxA % cellMap) - (idxB % cellMap));
        int dz = std::abs((idxA / cellMap) - (idxB / cellMap));
        return dx + dz;
    }

    inline vec3 indexToLoc(int idx) const
    {
        int gridX = (idx % cellMap) - half;
        int gridZ = (idx / cellMap) - half;
        return vec3((float)gridX * cellSize, y_level, (float)gridZ * cellSize);
    }

    list<int> cellsAround(const vec3& center, float radius) {
        static thread_local list<int> cachedCells;

        int gridRadius = static_cast<int>(radius / cellSize + 0.5f);
        int size = (gridRadius * 2 + 1) * (gridRadius * 2 + 1);
        if(cachedCells.space()<size) cachedCells.reserve(size);
        cachedCells.clear();

        vec3 centerPos = posToGrid(center);
        int cx = centerPos.x();
        int cz = centerPos.z();

        for (int dz = -gridRadius; dz <= gridRadius; ++dz) {
            int z = cz + dz;
            for (int dx = -gridRadius; dx <= gridRadius; ++dx) {
                int x = cx + dx;

                int index = cellIndex(x, z);
                if (index >= 0 && index < cells.length()) {
                    cachedCells.push(index);
                }
            }
        }

        return cachedCells;
    }
    };

}
