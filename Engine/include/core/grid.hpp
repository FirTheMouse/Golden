#pragma once

#include <util/util.hpp>
#include<core/object.hpp>
#include<util/d_list.hpp>

namespace Golden
{

using Cell = g_ptr<d_list<g_ptr<Object>>>;

class Grid : public Object
{
public:
    float cellSize;
    int mapSize;
    int cellMap;
    int half;
    float y_level = 0;
    q_list<g_ptr<d_list<g_ptr<Object>>>> cells;
    map<std::string,list<int>> typeCache;

    Grid(float cellSize_, int mapSize_) : cellSize(cellSize_), mapSize(mapSize_) { 
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

    Cell getCell(const vec3& worldPos)
    {
    int gridX = std::round(worldPos.x() / cellSize);
    int gridZ = std::round(worldPos.z() / cellSize);
    return getCell(gridX, gridZ);
    }

    inline Cell getCell(int gridX, int gridZ) {
        int index = cellIndex(gridX,gridZ);
        if(index>=cells.length()) {
        //print("Grid::52 implment cell addition in the future");
        return nullptr;
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

    //Does not work
    inline vec3 indexToLoc(int idx) const
    {
        int gridX = (idx % cellMap) - half;
        int gridZ = (idx / cellMap) - half;
        return vec3((float)gridX, y_level, (float)gridZ);
    }

    list<Cell> cellsAround(const vec3& center, float radius) {
        static thread_local list<Cell> cachedCells;

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
                    cachedCells.push(cells[index]);
                }
            }
        }

        return cachedCells;
    }

    // int cellBetween(int fromIdx, int toIdx, float maxDistance) {
    //     g_ptr<Cell>  from = cells[fromIdx];
    //     g_ptr<Cell>  to = cells[toIdx];

    //     int dx = to->x - from->x;
    //     int dz = to->z - from->z;

    //     float dist = std::sqrt(dx * dx + dz * dz) * cellSize;
    //     if (dist <= maxDistance)
    //         return toIdx;

    //     float ratio = maxDistance / dist;
    //     int stepX = static_cast<int>(std::round(from->x + dx * ratio));
    //     int stepZ = static_cast<int>(std::round(from->z + dz * ratio));

    //     return cellIndex(stepX, stepZ);
    // }

    // void cacheType(const std::string& type,int idx)
    // {
    //    if(!typeCache.hasKey(type)) {
    //       typeCache.put(type,std::unordered_set<int>{idx});
    //    }
    //    else typeCache.get(type).insert(idx);
    // }

    // std::unordered_set<int> fallback;
    // std::unordered_set<int>& getType(const std::string& type)
    // {
    //     if(typeCache.hasKey(type))
    //     return typeCache.get(type);
    //     else
    //     return fallback;
    // }


    // void Grid::scanCells(vec3 center, float radius) {
    //     static thread_local std::unique_ptr<uint8_t[]> checked;
    //     static thread_local bool initialized = false;

    //     int gridSize = cellMap * cellMap;

    //     if (!initialized) {
    //         checked = std::make_unique<uint8_t[]>(gridSize);
    //         std::fill(checked.get(), checked.get() + gridSize, 0);
    //         initialized = true;
    //     }

    //     // Signal to clear the cache
    //     if (radius < 0.0f) {
    //         std::fill(checked.get(), checked.get() + gridSize, 0);
    //         return;
    //     }

    //     int gridRadius = static_cast<int>(radius / cellSize + 0.5f);
    //     g_ptr<Cell>  centerCell = getCell(center);
    //     int minZ = centerCell->z - gridRadius;
    //     int maxZ = centerCell->z + gridRadius;
    //     int minX = centerCell->x - gridRadius;
    //     int maxX = centerCell->x + gridRadius;

    //     for (int z = minZ; z <= maxZ; ++z) {
    //         for (int x = minX; x <= maxX; ++x) {
    //             int idx = cellIndex(x, z);
    //             if (!checked[idx]) {
    //                 //   for(std::string s : cells[idx].types)
    //                 //     cout << s << endl;
    //                 Grid::checkCell(idx);
    //                 checked[idx] = 1;
    //             }
    //         }
    //     }
    // }

    // void checkCell(int idx)
    // {
    //     for(const std::string& s : cells[idx]->types) {cacheType(s,idx);}
    //     // std::unordered_set<std::string> typeSet(cells[idx].types.begin(),cells[idx].types.end());
    //     // for(const std::string& s : typeCache.keySet()) {if(typeSet.find(s)==typeSet.end()) typeCache.get(s).erase()}
    // }

    // template<typename Func>
    // int findType(const std::string& type,int point,Func&& f) {
    //     //const std::unordered_set<int>& typeSet = getType(type);
    //     return f(getType(type),point);
    // }

    };

}
