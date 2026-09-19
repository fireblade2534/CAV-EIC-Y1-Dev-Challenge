#include <antworld.h>

void FollowingPheromoneTrail::onChangeFrom(Ant &ant, AntWorld *world) {}

/*
   Scan the closest food source and assign it to the ant.
*/
void FollowingPheromoneTrail::onChangeTo(Ant &ant, AntWorld *world) {
    this->foodTarget = {-1, -1};

    // scan the upper half
    for (int i = ant.position.first; i <= ant.position.first + ant.pheromoneRadius; ++i) {
        for (int j = ant.position.second - ant.pheromoneRadius; j <= ant.position.second + ant.pheromoneRadius; ++j) {
            if (i < 0 || i >= world->pheromoneMap.size() || j < 0 ||
                j >= world->pheromoneMap[0].size()) {
                continue;
            }

            if (world->pheromoneMap[i][j].second != 0) {
                this->foodTarget = {i, j};
            }
        }
    }

    // scan the lower half
    for (int i = ant.position.first - 1;
         i >= ant.position.first - ant.pheromoneRadius; --i) {
        for (int j = ant.position.second - ant.pheromoneRadius;
             j <= ant.position.second + ant.pheromoneRadius; ++j) {
            if (i < 0 || i >= world->pheromoneMap.size() || j < 0 ||
                j >= world->pheromoneMap[0].size()) {
                continue;
            }

            if (world->pheromoneMap[i][j].second != 0) {
                this->foodTarget = {i, j};
            }
        }
    }
}

/*
   @brief: If there is a food location set, go to that location. If not or if
   impossible to get there, transition to exploration. If ant is there and has
   the food, transition to return home.
*/
void FollowingPheromoneTrail::onTick(Ant &ant, AntWorld *world) {
    /*
       TODO: write a system to determine which food spot to go for based on food
       pheromone and the trail pheromone of other ants nearby.

       If there is another ant that is closer to the food source, chances are
       it's going to take that and you can go find another food pheromone or go
       explore some more. => Voronoi partition for each ant
    */

    if (this->foodTarget.first == -1 || this->foodTarget.second == -1) {
        ant.switchTo(RandomExploration{}, world);
    } else if (ant.position == this->foodTarget && ant.carryingFood) {
        ant.switchTo(ReturningToHub{}, world);
    }

    auto paths =
        shortestPath(world->terrainMap, ant.position, this->foodTarget);
    paths.erase(paths.begin());

    auto before = ant.position;
    auto after = ant.move(world->terrainMap, *paths.begin(), world->foodMap);

    if (before == after && ant.energy != 0) {
        // cannot possibly reach the food, transition to random exploration
        // again
        ant.switchTo(RandomExploration{}, world);
    }
}
