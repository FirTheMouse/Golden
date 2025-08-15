#pragma once

#include <util/util.hpp>
#include<core/object.hpp>
#include<util/d_list.hpp>

namespace Golden
{

using Cell = g_ptr<d_list<g_ptr<Object>>>;

class Grid2D : public Object
{
public:
    float cellSize;
    int mapSize;
    int cellMap;
    int half;
    float y_level = 0;
    q_list<g_ptr<d_list<g_ptr<Object>>>> cells;
    map<std::string,list<int>> typeCache;

    Grid2D(float cellSize_, int mapSize_) : cellSize(cellSize_), mapSize(mapSize_) { 
        cellMap = static_cast<int>(mapSize / cellSize);
        float mapArea = cellMap*cellMap;
        cells.impl.reserve(mapArea);
        for(int i=0;i<mapArea;i++)
        {
            auto cell = make<d_list<g_ptr<Object>>>();
            cell->set<int>("id",cells.length());
            cells.push(cell);
        }
        half = cellMap/2;
    }

    Cell getCell(const vec2& worldPos)
    {
    int gridX = std::round(worldPos.x() / cellSize);
    int gridZ = std::round(worldPos.y() / cellSize);
    return getCell(gridX, gridZ);
    }

    inline Cell getCell(int gridX, int gridZ) {
        int index = cellIndex(gridX,gridZ);
        if(index>=cells.length()) {
        // print("Grid::52 implment cell addition in the future");
        return make<d_list<g_ptr<Object>>>();
        }
        return cells.get(index);
    }

    inline int cellIndex(int x, int z) const {
        return (z + half) * cellMap + (x + half);
    }

    int toIndex(const vec2& worldPos)
    {
    int gridX = std::round(worldPos.x() / cellSize);
    int gridZ = std::round(worldPos.y() / cellSize);
    return cellIndex(gridX,gridZ);
    }

    vec2 posToGrid(const vec2& worldPos)
    {
    int gridX = std::round(worldPos.x() / cellSize);
    int gridZ = std::round(worldPos.y() / cellSize);
    return vec2(gridX,gridZ);
    }

    vec2 snapToGrid(const vec2& worldPos) const {
        int gridX = std::round(worldPos.x() / cellSize);
        int gridZ = std::round(worldPos.y() / cellSize);
        return vec2(gridX * cellSize, gridZ * cellSize);
    }

    inline int cellDistance(int idxA, int idxB) const
    {
        int dx = std::abs((idxA % cellMap) - (idxB % cellMap));
        int dz = std::abs((idxA / cellMap) - (idxB / cellMap));
        return dx + dz;
    }

    inline vec2 indexToLoc(int idx) const
    {
        int gridX = (idx % cellMap) - half;
        int gridZ = (idx / cellMap) - half;
        return vec2((float)gridX, (float)gridZ);
    }

    list<Cell> cellsAround(const vec2& center, float radius) {
        static thread_local list<Cell> cachedCells;

        int gridRadius = static_cast<int>(radius / cellSize + 0.5f);
        int size = (gridRadius * 2 + 1) * (gridRadius * 2 + 1);
        if(cachedCells.space()<size) cachedCells.reserve(size);
        cachedCells.clear();

        vec2 centerPos = posToGrid(center);
        int cx = centerPos.x();
        int cz = centerPos.y();

        for (int dz = -gridRadius; dz <= gridRadius; ++dz) {
            int z = cz + dz;
            for (int dx = -gridRadius; dx <= gridRadius; ++dx) {
                int x = cx + dx;

                int index = cellIndex(x, z);
                if (index >= 0 && index < cells.length()) {
                    cachedCells.push(cells[index]);
                }
            }
        }

        return cachedCells;
    }
    };
}