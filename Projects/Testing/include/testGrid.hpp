#pragma once

#include<extension/grid.hpp>
#include<util/FastNoiseLite.h>

namespace Golden
{

class T_Cell : virtual public Cell
{
public:
    using Cell::Cell;

    float fertility = 0.0f;
};

class T_Grid : virtual public Grid
{
public:
    using Grid::Grid;

    void intializeGrid() override {
        FastNoiseLite texNoise;
        texNoise.SetSeed(1234);
        texNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
        texNoise.SetFrequency(0.05f);

        FastNoiseLite fertNoise;
        fertNoise.SetSeed(2025);
        fertNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
        fertNoise.SetFrequency(0.008f);

        FastNoiseLite clayNoise;
        clayNoise.SetSeed(2019);
        clayNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
        clayNoise.SetFrequency(0.008f);

        for(int r=-half;r<half;r++)
        {
            for(int c=-half;c<half;c++)
            {
                g_ptr<T_Cell> cell = make<T_Cell>(r,c,cellSize); 
                float texValue = texNoise.GetNoise((float)r, (float)c);
                float fertValue = fertNoise.GetNoise((float)r, (float)c);
                float clayValue = clayNoise.GetNoise((float)r, (float)c);

                // Normalize and compute fertility
                float cl = (clayValue + 1.0f) * 0.5f;
                float t = (texValue + 1.5f) * 0.2f;
                float fc = (fertValue + 1.f) * 0.4f;
                float f = (fc * fc) + 0.2f;
                float fn = (fertValue + 1.0f) * 0.5f;
                float f2 = (fn * fn) + 0.15f;
                cell->fertility = f2;

                // Adjust color based on fertility
                if (fc < 0.4f) f = 1.5f;
                else if (fc < 0.6f) f = 1.0f;
                else if (fc < 0.8f) f = 0.5f;

                if(cl>0.8f) cl= 0.02f;
                else cl = 0.0f;
                glm::vec4 color = glm::vec4(0.4f+(cl*2),0.2f-cl,0.1f-cl,1.0f) * (f+t);
                colorCell(cell,color);
                int index = cellIndex(r,c);
                cell->index = index;
                cells[index] = std::move(cell);
            }
        }
    }
};

}