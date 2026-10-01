#include <antworld.h>

#define MAX_TRAIL 5

void FollowingPheromoneTrail::onChangeFrom(Ant &ant, AntWorld *world) {}

/*
   Scan the closest food source and assign it to the ant.
*/
void FollowingPheromoneTrail::onChangeTo(Ant &ant, AntWorld *world) {
    this->pheromoneTarget = {-1, -1};

    // Scan using pheromone scan function
    auto foodTrails =
        ant.pheromoneScan(world->pheromoneMap, PheromoneType::Food);

    // Everything has faded
    if (foodTrails.empty()) {
        ant.switchTo(DeterminedExploration{}, world);
        return;
    }

    // Sort for weakest pheromone strength first
    sort(foodTrails.begin(), foodTrails.end(),
         [world](const Coord &a, const Coord &b) -> bool {
             return world->pheromoneMap[a.first][a.second].second <
                    world->pheromoneMap[b.first][b.second].second;
         });

    auto others = ant.pheromoneScan(world->pheromoneMap, PheromoneType::Position);

    /*
       Decides which pheromone location to go to and compute path. the rule for
       this is that for any location where there is another ant closer to that
       location than itself, the current ant is going to yield and look for
       another pheromone spot where it is the closest one. If no optimal
       location is found, it goes for the one with the weakest pheromone
       strength regardless. This is to avoid situations where all ants yielded
       and no pheromones are followed.

       Another problem is that since the map is tiny, there will be alot of food
       pheromone lingering around. We should only filter for pheromones that
       have a certain strengh, since those will be the ones closer to the food
       source.
    */

    Coord pheromoneTarget = ant.chooseTarget(foodTrails, others, ant.foodRadius, world->rng, false);
    if (pheromoneTarget.first == -1 || pheromoneTarget.second == -1) {
        // Get the first regardless
        this->pheromoneTarget = *foodTrails.begin();
    } else {
        this->pheromoneTarget = pheromoneTarget;
    }
    

    this->explorePath =
        shortestPath(world->terrainMap, ant.position, this->pheromoneTarget);
    this->explorePath.erase(this->explorePath.begin());
    this->currentStep = 0;
}

void FollowingPheromoneTrail::beforeTick(Ant &ant, AntWorld *world) {
    ant.dropPheromone(world->pheromoneMap, PheromoneType::Position, 1);
}

/*
   @brief: If there is a food location set, go to that location. If not or if
   impossible to get there, transition to exploration. If ant is there and has
   the food, transition to return home.
*/
void FollowingPheromoneTrail::onTick(Ant &ant, AntWorld *world) {
    if (this->pheromoneTarget.first == -1 ||
        this->pheromoneTarget.second == -1) {
        logStateTransition(ant.antID, "FollowingPheromoneTrail", "DeterminedExploration",
                           "no valid pheormone location");

        ant.switchTo(DeterminedExploration{}, world);
        return;
    } else if (ant.position == this->pheromoneTarget) {
        if (ant.carryingFood) {
            logStateTransition(ant.antID, "FollowingPheromoneTrai", "ReturningToHub",
                               "food spotted at pheromone location");

            ant.switchTo(ReturningToHub{}, world);
            return;
        } else {
            logStateTransition(ant.antID, "FollowingPheromoneTrail",
                               "DeterminedExploration",
                               "reached pheromone location but no food found");

            ant.switchTo(DeterminedExploration{}, world);
            return;
        }
    }

    // Scan for food while following trail
    auto foods = ant.foodScan(world->foodMap);
    auto positionPheromones = ant.pheromoneScan(world->pheromoneMap, PheromoneType::Position);

    Coord foodChoice = ant.chooseTarget(foods, positionPheromones, ant.foodRadius, world->rng);

    if (foodChoice.first != -1 && foodChoice.second != -1) {
        logStateTransition(ant.antID, "FollowingPheromoneTrail", "FoundFood",
                           "found food at (%d, %d) after following trail", foodChoice.first,
                           foodChoice.second);

        ant.switchTo(FoundFood{.path = {}, .food = foodChoice}, world);
        return;
    }

    auto before = ant.position;
    if (before == ant.move(world->terrainMap,
                           this->explorePath[this->currentStep++],
                           world->foodMap)) {
        logStateTransition(ant.antID, "FollowingPheromoneTrail", "DeterminedExploration",
                           "cannot reach pheromone location");

        ant.switchTo(DeterminedExploration{}, world);
        return;
    }
}
