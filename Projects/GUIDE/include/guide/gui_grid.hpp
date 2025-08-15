#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <util/util.hpp>
#include <rendering/model.hpp>
#include <unordered_set>
#include <rendering/scene_object.hpp>
#include <guide/gui_cell_object.hpp>

namespace Golden
{

class Cell
{
public:
    int x,z;
    float cellSize;
    int index;
    std::vector<C_Object*> objects;
    std::vector<std::string> types;
    bool solid = false;
    Cell() = default;  
    Cell(int x_, int z_, float cellSize_) : x(x_), z(z_), cellSize(cellSize_) {}

    vec3 getPosition() const {
        return vec3(x * cellSize, z * cellSize, 0);
    }

    void add(C_Object* object);

    void remove(C_Object* object);

    bool hasType(std::string type);

    std::vector<C_Object*> getType(std::string type);
};

class Grid
{
public:
    float cellSize;
    int mapSize;
    int cellMap;
    int half;
    std::vector<Cell> cells;
    map<std::string,std::unordered_set<int>> typeCache;
    list<int> chunkSizes{2, 4, 6, 8, 12, 16};


    Grid(float cellSize_, int mapSize_) : cellSize(cellSize_), mapSize(mapSize_) {  
        cellMap = static_cast<int>(mapSize / cellSize);
        cells.resize(cellMap * cellMap);
        half = cellMap/2;

        for(int r=-half;r<half;r++)
        {
	        for(int c=-half;c<half;c++)
	        {
                Cell cell(r,c,cellSize); 
                int index = cellIndex(r,c);
                cell.index = index;
                cells[index] = std::move(cell);
            }
        }
    }

    Cell& getCell(const vec3& worldPos)
    {
    int gridX = std::round(worldPos.x() / cellSize);
    int gridY = std::round(worldPos.y() / cellSize);
    return getCell(gridX, gridY);
    }

    inline int cellIndex(int x, int y) const {
        return (y + half) * cellMap + (x + half);
    }

    int toIdx(const vec3& worldPos)
    {
    int gridX = std::round(worldPos.x() / cellSize);
    int gridY = std::round(worldPos.y() / cellSize);
    return cellIndex(gridX,gridY);
    }

    inline Cell& getCell(int gridX, int gridY) {
        int index = cellIndex(gridX,gridY);
        if(index>=cells.size()) {
        std::cerr << "getCell: cell beyond grid! Depracated fallback triggered, replace later\n";
        return cells[0];}
        return cells[index];
    }

    inline int cellDistance(int idxA, int idxB) const
    {
        int dx = std::abs((idxA % cellMap) - (idxB % cellMap));
        int dz = std::abs((idxA / cellMap) - (idxB / cellMap));
        return dx + dz;
    }

    inline vec3 idxLoc(int idx) const
    {
        return vec3((float)(idx%cellMap),0.0f,(float)(idx)/cellMap);
    }

    vec3 toLoc(int idx) const
    {
        return cells[idx].getPosition();
    }

    inline int cellDistance(Cell cellA, Cell cellB) const
    {
        int dx = std::abs(cellA.x - cellB.x);
        int dz = std::abs(cellA.z - cellB.z);
        return dx + dz;
    }


    int cellBetween(int fromIdx, int toIdx, float maxDistance) {
        Cell& from = cells[fromIdx];
        Cell& to = cells[toIdx];

        int dx = to.x - from.x;
        int dz = to.z - from.z;

        float dist = std::sqrt(dx * dx + dz * dz) * cellSize;
        if (dist <= maxDistance)
            return toIdx;

        float ratio = maxDistance / dist;
        int stepX = static_cast<int>(std::round(from.x + dx * ratio));
        int stepZ = static_cast<int>(std::round(from.z + dz * ratio));

        return cellIndex(stepX, stepZ);
    }

    void cacheType(const std::string& type,int idx)
    {
       if(!typeCache.hasKey(type)) {
          typeCache.put(type,std::unordered_set<int>{idx});
       }
       else typeCache.get(type).insert(idx);
    }

    std::unordered_set<int> fallback;
    std::unordered_set<int>& getType(const std::string& type)
    {
        if(typeCache.hasKey(type))
        return typeCache.get(type);
        else
        return fallback;
    }

    void scanCells(vec3 center, float radius);

    void checkCell(int idx)
    {
        for(const std::string& s : cells[idx].types) {cacheType(s,idx);}
        // std::unordered_set<std::string> typeSet(cells[idx].types.begin(),cells[idx].types.end());
        // for(const std::string& s : typeCache.keySet()) {if(typeSet.find(s)==typeSet.end()) typeCache.get(s).erase()}
    }

    template<typename Func>
    int findType(const std::string& type,int point,Func&& f) {
        //const std::unordered_set<int>& typeSet = getType(type);
        return f(getType(type),point);
    }

std::vector<Cell*>& cellsAround(const vec3& center, float radius) {
    static thread_local std::vector<Cell*> cachedCells;

    int gridRadius = static_cast<int>(radius / cellSize + 0.5f); // faster than round
    int size = (gridRadius * 2 + 1) * (gridRadius * 2 + 1);
    if (cachedCells.capacity() < size) cachedCells.reserve(size);
    cachedCells.clear();

    Cell centerCell = getCell(center);
    int cx = centerCell.x;
    int cz = centerCell.z;

    for (int dz = -gridRadius; dz <= gridRadius; ++dz) {
        int z = cz + dz;
        for (int dx = -gridRadius; dx <= gridRadius; ++dx) {
            int x = cx + dx;

            int index = cellIndex(x, z);
            if (index >= 0 && index < static_cast<int>(cells.size())) {
                cachedCells.push_back(&cells[index]);
            }
        }
    }

    return cachedCells;
}
};

}
