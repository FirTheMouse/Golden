#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <util/util.hpp>
#include <rendering/single.hpp>
#include <unordered_set>
#include <rendering/scene_object.hpp>
#include <extension/cell_object.hpp>

namespace Golden
{

class C_Object;

class Cell : virtual public Object
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
        return vec3(x * cellSize, 0, z * cellSize);
    }

    void add(C_Object* object);

    void remove(C_Object* object);

    bool hasType(std::string type);

    std::vector<C_Object*> getType(std::string type);
};

class Grid : virtual public Object, virtual public Single
{
public:
    float cellSize;
    int mapSize;
    int cellMap;
    int half;
    std::vector<g_ptr<Cell>> cells;
    //Cell cells[100];
    map<std::string,std::unordered_set<int>> typeCache;
    list<int> chunkSizes{2, 4, 6, 8, 12, 16};


    Grid(float cellSize_, int mapSize_,Model&& _model) : cellSize(cellSize_), mapSize(mapSize_) { 
        model = make<Model>(std::move(_model));
        cellMap = static_cast<int>(mapSize / cellSize);
        cells.resize(cellMap * cellMap);
        half = cellMap/2;
    }

    virtual void intializeGrid()
    {
        for(int r=-half;r<half;r++)
        {
	        for(int c=-half;c<half;c++)
	        {
                g_ptr<Cell> cell = make<Cell>(r,c,cellSize); 
                int index = cellIndex(r,c);
                cell->index = index;
                cells[index] = std::move(cell);
            }
        }
    }

    g_ptr<Cell> getCell(const vec3& worldPos)
    {
    int gridX = std::round(worldPos.x() / cellSize);
    int gridZ = std::round(worldPos.z() / cellSize);
    return getCell(gridX, gridZ);
    }

    inline int cellIndex(int x, int z) const {
        return (z + half) * cellMap + (x + half);
    }

    int toIdx(const vec3& worldPos)
    {
    int gridX = std::round(worldPos.x() / cellSize);
    int gridZ = std::round(worldPos.z() / cellSize);
    return cellIndex(gridX,gridZ);
    }

    inline g_ptr<Cell> getCell(int gridX, int gridZ) {
        int index = cellIndex(gridX,gridZ);
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
        return cells[idx]->getPosition();
    }

    inline int cellDistance(Cell cellA, Cell cellB) const
    {
        int dx = std::abs(cellA.x - cellB.x);
        int dz = std::abs(cellA.z - cellB.z);
        return dx + dz;
    }

    inline int cellDistance(g_ptr<Cell> cellA, g_ptr<Cell> cellB) const
    {
        int dx = std::abs(cellA->x - cellB->x);
        int dz = std::abs(cellA->z - cellB->z);
        return dx + dz;
    }

    vec3 stepPath(std::vector<int>& path,float speed)
    {
        int gridSpeed = std::round(speed/cellSize);
        int moveamt = gridSpeed>path.size() ? path.size() : gridSpeed;
        //DEPRACATED FALLBACK REPLACE LATER
        if(path.size()<=0) return vec3(0,0,0);
        path.erase(path.begin(), path.begin() + moveamt);
        return toLoc(path.front());
    }

    vec3 lookAheadPath(std::vector<int>& path,float speed)
    {
        int gridSpeed = std::round(speed/cellSize);
        int moveamt = gridSpeed>path.size() ? path.size() : gridSpeed;
        return toLoc(path.at(moveamt));
    }

    int cellBetween(int fromIdx, int toIdx, float maxDistance) {
        g_ptr<Cell>  from = cells[fromIdx];
        g_ptr<Cell>  to = cells[toIdx];

        int dx = to->x - from->x;
        int dz = to->z - from->z;

        float dist = std::sqrt(dx * dx + dz * dz) * cellSize;
        if (dist <= maxDistance)
            return toIdx;

        float ratio = maxDistance / dist;
        int stepX = static_cast<int>(std::round(from->x + dx * ratio));
        int stepZ = static_cast<int>(std::round(from->z + dz * ratio));

        return cellIndex(stepX, stepZ);
    }

    //This is slow and needs to be replaced later
    std::unordered_set<int> cellCast(int fromIdx, int toIdx) {
        g_ptr<Cell>  from = cells[fromIdx];
        g_ptr<Cell>  to = cells[toIdx];
        std::unordered_set<int> toReturn;
        vec3 dir = from->getPosition().direction(to->getPosition());
        int dist = cellDistance(from,to);
        for(int i=0;i<dist;i++)
        {
            vec3 d = from->getPosition()+(dir.mult(cellSize*i));
            toReturn.insert(this->toIdx(d));
        }
        return toReturn;
    }


    void colorCell(g_ptr<Cell> cell, const glm::vec4& newColor) {
        colorCell(cell->getPosition(),newColor);
    }

    void colorCell(vec3 pos, const glm::vec4& newColor) {
        // Convert world space to vertex grid index
        int vertexX = static_cast<int>((pos.x() + mapSize * 0.5f) / cellSize + 0.5f);
        int vertexZ = static_cast<int>((pos.z() + mapSize * 0.5f) / cellSize + 0.5f);

        // Optional clamping (skip if you're sure it's in range)
        if (vertexX < 0 || vertexX > cellMap || vertexZ < 0 || vertexZ > cellMap) return;

        int index = vertexZ * (cellMap + 1) + vertexX;
        auto& mesh = model->meshes[0];

        // Update CPU-side
        mesh.vertices[index].color = newColor;

        // Update GPU-side
        constexpr size_t colorOffset = offsetof(Vertex, color);
        size_t byteOffset = index * sizeof(Vertex) + colorOffset;
        glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
        glBufferSubData(GL_ARRAY_BUFFER, byteOffset, sizeof(glm::vec4), glm::value_ptr(newColor));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
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
        for(const std::string& s : cells[idx]->types) {cacheType(s,idx);}
        // std::unordered_set<std::string> typeSet(cells[idx].types.begin(),cells[idx].types.end());
        // for(const std::string& s : typeCache.keySet()) {if(typeSet.find(s)==typeSet.end()) typeCache.get(s).erase()}
    }

    template<typename Func>
    int findType(const std::string& type,int point,Func&& f) {
        //const std::unordered_set<int>& typeSet = getType(type);
        return f(getType(type),point);
    }

    std::vector<g_ptr<Cell>> cellsAround(const vec3& center, float radius) {
        static thread_local std::vector<g_ptr<Cell>> cachedCells;

        int gridRadius = static_cast<int>(radius / cellSize + 0.5f); // faster than round
        int size = (gridRadius * 2 + 1) * (gridRadius * 2 + 1);
        if (cachedCells.capacity() < size) cachedCells.reserve(size);
        cachedCells.clear();

        g_ptr<Cell> centerCell = getCell(center);
        int cx = centerCell->x;
        int cz = centerCell->z;

        for (int dz = -gridRadius; dz <= gridRadius; ++dz) {
            int z = cz + dz;
            for (int dx = -gridRadius; dx <= gridRadius; ++dx) {
                int x = cx + dx;

                int index = cellIndex(x, z);
                if (index >= 0 && index < static_cast<int>(cells.size())) {
                    cachedCells.push_back(cells[index]);
                }
            }
        }

        return cachedCells;
    }
    };

}
