#pragma once

#include <iostream>
#include <vector>
#include <rendering/scene_object.hpp>

namespace Golden
{
class Grid;
class C_Object : virtual public S_Object
{
public:
    C_Object() = default;
    std::vector<int> cells;
    Grid* grid;

    virtual void setGrid(Grid* g) {grid = g;}

    void remove() override;

    void updateCells();
    void updateCells(float radius);
};


}