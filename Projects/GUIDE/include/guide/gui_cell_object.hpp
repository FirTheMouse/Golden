#pragma once

#include <util/util.hpp>
#include <iostream>
#include <vector>
#include <rendering/scene_object.hpp>

namespace Golden
{
class Grid;
class C_Object : virtual public S_Object
{
public:
    C_Object() {}
    ~C_Object() {}
    std::vector<int> cells;
    Grid* grid;

    void remove() override;

    void updateCells();
    void updateCells(float radius);
};


}