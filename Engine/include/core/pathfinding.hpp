#pragma once

#include <glm/glm.hpp>
#include <queue>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <chrono>

namespace Golden
{

struct Node {
    int index;
    float g;  // cost so far
    float f;  // g + heuristic
    int cameFrom;
};

struct NodeCompare {
    bool operator()(const Node& a, const Node& b) const {
        return a.f > b.f;  // min-heap (lowest f first)
    }
};

class Pathfinder
{
public:
    Pathfinder() {}

    virtual float heuristic(int a, int b) = 0;

    virtual float cost(int a, int b) = 0;

    virtual std::vector<int> getWalkableNeighbors(int index) = 0;

    virtual bool isSurrounded(int originIdx, float radius) = 0;

    virtual int closestViable(int originIdx, int goalIDx, float width) = 0;

    std::vector<int> reconstructPath(std::unordered_map<int, int>& cameFrom, int current)
    {
        std::vector<int> path;
        while (cameFrom.contains(current)) {
            path.push_back(current);
            current = cameFrom[current];
        }
        std::reverse(path.begin(), path.end());
        return path;
    }

};

std::vector<int> findPath(int startIdx, int goalIdx,Pathfinder& pathe,float width);



}