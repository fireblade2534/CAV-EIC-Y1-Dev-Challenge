#include <antworld.h>

int energyCost(MapTemplate &terrainMap, Coord begin, Coord dest) {
    if ((size_t)dest.first >= terrainMap.size() ||
        (size_t)dest.second >= terrainMap[0].size()) {
        return -1;
    }

    std::vector<Coord> path = shortestPath(terrainMap, begin, dest);
    return calculatePathCost(terrainMap, path);
}

/*
   This function has the highest priority on the state machine because it
   guarantees that there is food.

   @returns: Returns true if ant makes it home, false if dies on the way
*/
bool Ant::followPheromoneTrail(MapTemplate &terrainMap, MapTemplate &foodMap,
                               PheromoneTemplate pheromoneMap) {
    auto trails = this->pheromoneScan(pheromoneMap, PheromoneType::Food);

    /*
       TODO: write a system to determine which food spot to go for based on food pheromone and the trail pheromone of other ants nearby.

       If there is another ant that is closer to the food source, chances are it's going to take that and you can go find another food pheromone or go explore some more.
    */

    /* sort by closest distance */
    std::sort(trails.begin(), trails.end(),
              [&](const Coord &a, const Coord &b) -> bool {
                  return energyCost(terrainMap, this->position, a) <
                         energyCost(terrainMap, this->position, b);
              });

    for (auto trail : trails) {

        Coord finalPos = this->move(terrainMap, trail, foodMap);

        if (finalPos == trail) {
            this->dropPheromone(pheromoneMap, PheromoneType::Trail);
            this->returnHome(terrainMap, foodMap);
            return true;
        } else {
            // it's dead, so drop food pheromone here for another ant to pick up
            this->dropPheromone(pheromoneMap, PheromoneType::Food);
            return false;
        }
    }

    return true;
}
