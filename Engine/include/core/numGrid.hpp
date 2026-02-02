#pragma once

#include <util/engine_util.hpp>
#include<core/object.hpp>

namespace Golden
{

    class NumGrid : public Object
    {
    public:
        float cellSize;
        int mapSize;
        int cellMap;
        int half;
        float y_level = 0;
        list<list<int>> cells;
        map<std::string,list<int>> typeCache;

        NumGrid(float cellSize_, int mapSize_) : cellSize(cellSize_), mapSize(mapSize_) { 
            cellMap = static_cast<int>(mapSize / cellSize);
            float mapArea = cellMap*cellMap;
            cells.reserve(mapArea);
            for(int i=0;i<mapArea;i++)
            {
                cells.push(list<int>());
            }
            half = cellMap/2;
        }

        list<int> getCell(const vec3& worldPos)
        {
        int gridX = std::round(worldPos.x() / cellSize);
        int gridZ = std::round(worldPos.z() / cellSize);
        return getCell(gridX, gridZ);
        }

        inline list<int> getCell(int gridX, int gridZ) {
            int index = cellIndex(gridX,gridZ);
            if(index>=cells.length()) {
            //print("Grid::52 implment cell addition in the future");
            return list<int>{};
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

        inline vec3 indexToLoc(int idx) const
        {
            int gridX = (idx % cellMap) - half;
            int gridZ = (idx / cellMap) - half;
            return vec3((float)gridX * cellSize, y_level, (float)gridZ * cellSize);
        }

        list<int> cellsAround(const BoundingBox& bounds) {
            static thread_local list<int> cachedCells;
            
            // Convert bounds to grid coordinates
            int minX = std::floor(bounds.min.x() / cellSize);
            int maxX = std::ceil(bounds.max.x() / cellSize);
            int minZ = std::floor(bounds.min.z() / cellSize);
            int maxZ = std::ceil(bounds.max.z() / cellSize);
            
            int width = maxX - minX + 1;
            int height = maxZ - minZ + 1;
            int size = width * height;
            
            if(cachedCells.space() < size) cachedCells.reserve(size);
            cachedCells.clear();
            
            for(int z = minZ; z <= maxZ; ++z) {
                for(int x = minX; x <= maxX; ++x) {
                    int index = cellIndex(x, z);
                    if(index >= 0 && index < cells.length()) {
                        cachedCells.push(index);
                    }
                }
            }
            
            return cachedCells;
        }
        
        // Keep the old one too for convenience
        list<int> cellsAround(const vec3& center, float radius) {
            BoundingBox bounds(
                center - vec3(radius, 0, radius),
                center + vec3(radius, 0, radius)
            );
            return cellsAround(bounds);
        }

        list<int> addToGrid(int obj_idx,const BoundingBox& bounds) {
            list<int> around = cellsAround(bounds);
            for(auto cell : around) {cells[cell].push(obj_idx);}
            return around;
        }

        list<int> removeFromGrid(int obj_idx,const BoundingBox& bounds) {
            list<int> around = cellsAround(bounds);
            for(auto cell : around) {cells[cell].erase(obj_idx);}
            return around;
        }

    // Returns a pair of distance to first hit, or max_dist if no hit, as well as the index of the cell hit
    // exclude_ids: list of object IDs to ignore (e.g., the caster itself)
    std::pair<float,int> raycast(const vec3& origin, const vec3& direction, float max_dist, 
                const list<int>& exclude_ids = list<int>{}) {
        
        vec3 dir = direction.normalized();
        float step_size = cellSize * 0.5f; // Half cell for finer sampling
        int max_steps = (int)(max_dist / step_size) + 1;
        
        // Convert exclude list to set for O(1) lookup
        static thread_local list<int> seen_flags;
        if(seen_flags.length() < 100000) { // Adjust based on max object count
            seen_flags = list<int>(100000, 0);
        }
        
        // Mark exclusions with current raycast ID (generation counter)
        static int raycast_generation = 0;
        raycast_generation++;
        for(int exclude_id : exclude_ids) {
            if(exclude_id < seen_flags.length()) {
                seen_flags[exclude_id] = raycast_generation;
            }
        }
        
        for(int step = 0; step < max_steps; step++) {
            float dist = step * step_size;
            vec3 sample_point = origin + dir * dist;
            
            int cell_idx = toIndex(sample_point);
            if(cell_idx < 0 || cell_idx >= cells.length()) {
                return {max_dist,-1}; // Out of bounds
            }
            
            // Check if cell has any objects
            if(!cells[cell_idx].empty()) {
                // Check each object in cell
                for(int obj_id : cells[cell_idx]) {
                    // Skip excluded objects
                    if(obj_id < seen_flags.length() && 
                    seen_flags[obj_id] == raycast_generation) {
                        continue;
                    }
                    
                    // Found a hit!
                    return {dist,cell_idx};
                }
            }
        }
        
        return {max_dist,-1}; // No hit
    }

    // Returns a pair of distance to first hit, or max_dist if no hit, as well as the index of the cell hit
    std::pair<float,int> raycast(const vec3& origin, const vec3& direction, float max_dist, int exclude_id) {
        list<int> exclusions;
        exclusions.push(exclude_id);
        return raycast(origin, direction, max_dist, exclusions);
    }


        // Add to NumGrid class
        list<int> findPath(int startIdx, int goalIdx, std::function<bool(int)> isWalkable) {
            if(startIdx == goalIdx) return list<int>{goalIdx};
            if(!isWalkable(startIdx) || !isWalkable(goalIdx)) return list<int>{};
            
            map<int, int> cameFrom;
            map<int, float> gScore;
            map<int, float> fScore;
            map<int, bool> closed;
            list<int> openSet;
            
            gScore[startIdx] = 0.0f;
            fScore[startIdx] = (float)cellDistance(startIdx, goalIdx);
            openSet.push(startIdx);
            
            int iterations = 0;
            int max_iterations = 100000; // Safety limit
            
            while(!openSet.empty() && iterations < max_iterations) {
                iterations++;
                // Find node with lowest fScore
                int current = openSet[0];
                float lowestF = fScore.getOrDefault(current, std::numeric_limits<float>::max());
                for(int idx : openSet) {
                    float f = fScore.getOrDefault(idx, std::numeric_limits<float>::max());
                    if(f < lowestF) {
                        lowestF = f;
                        current = idx;
                    }
                }
                
                if(current == goalIdx) {
                    // Reconstruct path
                    list<int> path;
                    path.push(current);
                    while(cameFrom.hasKey(current)) {
                        current = cameFrom[current];
                        path.insert(current,0);
                    }
                    return path;
                }
                
                openSet.erase(current);
                closed[current] = true;
                
                // 4-way neighbors
                int neighbors[4] = {current - 1, current + 1, current - cellMap, current + cellMap};
                
                for(int neighbor : neighbors) {
                    if(neighbor < 0 || neighbor >= cells.length()) continue;
                    if(closed.getOrDefault(neighbor, false)) continue;
                    if(!isWalkable(neighbor)) continue;
                    
                    float tentativeG = gScore.getOrDefault(current, std::numeric_limits<float>::max()) + 1.0f;
                    
                    if(!openSet.has(neighbor)) {
                        openSet.push(neighbor);
                    } else if(tentativeG >= gScore.getOrDefault(neighbor, std::numeric_limits<float>::max())) {
                        continue;
                    }
                    
                    cameFrom[neighbor] = current;
                    gScore[neighbor] = tentativeG;
                    fScore[neighbor] = tentativeG + (float)cellDistance(neighbor, goalIdx);
                }
            }

            if(iterations >= max_iterations) {
                //print("A* exceeded iteration limit!");
                return list<int>{}; // Failed to find path
            }
            
            return list<int>{};
        }
    };
}
