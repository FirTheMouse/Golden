#pragma once

#include <glm/glm.hpp>
#include <queue>
#include <vector>
#include <grid.hpp>
#include <cell_object.hpp>
#include <core/pathfinding.hpp>

namespace Golden
{
class Grid_Pathfinder : virtual public Pathfinder
{
public:
    Grid& grid;
    Grid_Pathfinder(Grid&_grid) : grid(_grid) {}
    //Grid_Pathfinder(const Grid_Pathfinder& other) = default;

    void setGrid(Grid& _grid) {grid = _grid;}

    float heuristic(int a, int b) override {
        return (float)grid.cellDistance(a, b);  // Manhattan is great for grid
    }

    float cost(int a, int b) override {
        return grid.cells[b].solid ? INFINITY : 1.0f;
    }

    bool isSurrounded(int originIdx, float radius) override {
    std::vector<Cell*> nearby = grid.cellsAroundUltraFast(grid.toLoc(originIdx), radius);

    for (Cell* cell : nearby) {
        if (!cell->solid) {
            return false;  // There's at least one escape route
        }
    }

    return true;  // No way out
    }

    int closestViable(int originIdx, int goalIdx, float width) override
    {
       int index = grid.cellBetween(originIdx,goalIdx,width+(grid.cellSize));
       Cell& goal = grid.cells[goalIdx];
       if(grid.cells[index].solid)
       {
        std::vector<Cell*> nearby = grid.cellsAroundUltraFast(grid.toLoc(goalIdx), width + grid.cellSize);
        int closest = 0;
        int cDist = INFINITY;
        for (Cell* cell : nearby) {
            if (!cell->solid) {
                int dist = grid.cellDistance(*cell,goal);
                if(dist<cDist)
                {cDist = dist; closest = cell->index;}
            }
        }
        if(closest!=0)  return closest;
        else  return originIdx;
        //Extend this fallback later for if we're surrounded
       }
       else return index;
    }

    std::vector<int> getWalkableNeighbors(int index) override
    {
    std::vector<int> neighbors;
    const Cell& cell = grid.cells[index];
    for (int dx = -1; dx <= 1; ++dx) {
        for (int dz = -1; dz <= 1; ++dz) {
            if (std::abs(dx) + std::abs(dz) != 1) continue;  // no diagonals
            int nx = cell.x + dx;
            int nz = cell.z + dz;
            int nidx = grid.cellIndex(nx, nz);
            if (nidx >= 0 && nidx < (int)grid.cells.size() && !grid.cells[nidx].solid) {
                neighbors.push_back(nidx);
            }
        }
    }
    return neighbors;
    }

};
}