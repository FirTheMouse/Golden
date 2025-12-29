#include <core/pathfinding.hpp>


namespace Golden
{

std::vector<int> findPath(int startIdx, int goalIdx, Pathfinder& pather, float width) {

    //If stuck
   if (pather.cost(startIdx,startIdx)>=INFINITY || pather.isSurrounded(startIdx, width)) {
          startIdx = pather.closestViable(startIdx,goalIdx,width);
    }

        // Fallback check: goal is unreachable
    if (pather.cost(startIdx, goalIdx) >= INFINITY) {
        goalIdx = pather.closestViable(startIdx,goalIdx,width);
    }

    std::priority_queue<Node, std::vector<Node>, NodeCompare> openSet;
    std::unordered_map<int, float> gScore;
    std::unordered_map<int, float> fScore;
    std::unordered_map<int, int> cameFrom;

    openSet.push({startIdx, 0.0f, pather.heuristic(startIdx, goalIdx), -1});
    gScore[startIdx] = 0.0f;
    fScore[startIdx] = pather.heuristic(startIdx, goalIdx);


    auto startTime = std::chrono::steady_clock::now();
    while (!openSet.empty()&&
    std::chrono::duration<float>(std::chrono::steady_clock::now() - startTime).count() < 0.005f) {
        Node current = openSet.top();
        openSet.pop();

        if (current.index == goalIdx)
            return pather.reconstructPath(cameFrom, goalIdx);

        for (int neighbor : pather.getWalkableNeighbors(current.index)) {
            float tentativeG = gScore[current.index] + pather.cost(current.index, neighbor);
            if (!gScore.contains(neighbor) || tentativeG < gScore[neighbor]) {
                cameFrom[neighbor] = current.index;
                gScore[neighbor] = tentativeG;
                fScore[neighbor] = tentativeG + pather.heuristic(neighbor, goalIdx);
                openSet.push({neighbor, tentativeG, fScore[neighbor], current.index});
            }
        }
    }

    if(!openSet.empty()) std::cout << "STALLED PATHING" << std::endl;

    return {};
}


}