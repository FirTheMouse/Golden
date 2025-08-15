#pragma once

#include<extension/grid.hpp>
#include<util/FastNoiseLite.h>

namespace Golden
{

class H_Cell : virtual public Cell
{
public:
    using Cell::Cell;

    float fertility = 0.0f;
};

class H_Grid : virtual public Grid
{
public:
    using Grid::Grid;

    void intializeGrid() override {

        for(int r=-half;r<half;r++)
        {
            for(int c=-half;c<half;c++)
            {
                g_ptr<H_Cell> cell = make<H_Cell>(r,c,cellSize); 
                
                colorCell(cell,glm::vec4(0.1f,0.08f,0.08f,1));
                int index = cellIndex(r,c);
                cell->index = index;
                cells[index] = std::move(cell);
            }
        }
    }
};

}