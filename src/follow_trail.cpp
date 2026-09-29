#include <antworld.h>

void FollowingPheromoneTrail::onChangeFrom(Ant &ant, AntWorld *world) {}

/*
   Scan the closest food source and assign it to the ant.
*/
void FollowingPheromoneTrail::onChangeTo(Ant &ant, AntWorld *world) {
    this->foodTarget = {-1, -1};

    // scan using pheromone scan function
    auto foodTrails =
        ant.pheromoneScan(world->pheromoneMap, PheromoneType::Food);

    // everything has faded
    if (foodTrails.empty()) {
        ant.switchTo(DeterminedExploration{}, world);
        return;
    }

    // sort for weakest pheromone strength first
    sort(foodTrails.begin(), foodTrails.end(),
         [world](const Coord &a, const Coord &b) -> bool {
             return world->pheromoneMap[a.first][a.second].first <
                    world->pheromoneMap[b.first][b.second].first;
         });

    auto others = ant.pheromoneScan(world->pheromoneMap, PheromoneType::Trail);

    /*
       Decides which pheromone location to go to and compute path. the rule for
       this is that for any location where there is another ant closer to that
       location than itself, the current ant is going to yield and look for
       another pheromone spot where it is the closest one. If no optimal
       location is found, it goes for the one with the weakest pheromone
       strength regardless. This is to avoid situations where all ants yielded
       and no pheromones are picked up.
    */
    for (int i = 0; i < std::min(5, (int)foodTrails.size()); i++) {
        auto &loc = foodTrails[i];

        // if there's an ant closer, yield
        int selfDist = getManhattanDistance(ant.position, loc);
        bool closest = true;

        for (auto &other : others) {
            int dist = getManhattanDistance(other, loc);

            if (dist < selfDist) {
                closest = false;
                break;
            }
        }

        if (closest) {
            this->foodTarget = loc;
            break;
        }
    }

    if (this->foodTarget.first == -1 || this->foodTarget.second == -1) {
        // get the first regardless
        this->foodTarget = *foodTrails.begin();
    }
}

/*
   @brief: If there is a food location set, go to that location. If not or if
   impossible to get there, transition to exploration. If ant is there and has
   the food, transition to return home.
*/
void FollowingPheromoneTrail::onTick(Ant &ant, AntWorld *world) {
    if (this->foodTarget.first == -1 || this->foodTarget.second == -1) {
        ant.switchTo(DeterminedExploration{}, world);
        return;
    } else if (ant.position == this->foodTarget) {
        if (ant.carryingFood) {
            ant.switchTo(ReturningToHub{}, world);
            return;
        } else {
            ant.switchTo(DeterminedExploration{}, world);
            return;
        }
    }

    auto paths =
        shortestPath(world->terrainMap, ant.position, this->foodTarget);
    paths.erase(paths.begin());

    auto before = ant.position;
    if (before == ant.move(world->terrainMap, *paths.begin(), world->foodMap) &&
        ant.energy != 0) {
        // cannot possibly reach the food, transition to random exploration
        // again
        ant.switchTo(DeterminedExploration{}, world);
    }
}
