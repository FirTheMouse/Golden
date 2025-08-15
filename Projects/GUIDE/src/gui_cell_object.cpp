#include <guide/gui_cell_object.hpp>
#include <guide/gui_grid.hpp>

namespace Golden
{

void C_Object::updateCells(){
    // if(scene)
    // {
    // for(auto n : cells) grid->cells[n].remove(this);
    // cells.clear();
    // grid->getCell(getPosition()).add(this);
    // }
    }

void C_Object::updateCells(float radius){
    // if(scene)
    // {
    // for(auto n : cells) grid->cells[n].remove(this);
    // cells.clear();
    // for(auto c : grid->cellsAround(getPosition(),radius))
    // c->add(this);
    // }
}

 void C_Object::remove() {
    for(int n : cells)
    {
        grid->cells[n].remove(this);
    }
    }


}