#include <antworld.h>

void DeterminedExploration::onChangeTo(Ant &ant, AntWorld *world) {
    if (ant.position == ant.exploreDirection) {
        ant.exploreDirection =
            world->getNewExploreDirection(ant.exploreDirection);
    }

    this->explorePath =
        shortestPath(world->terrainMap, ant.position, ant.exploreDirection);
    if (this->explorePath.empty()) {
        #ifdef DEBUG_MOVEMENT
            printf("shortest path returned empty list, switching to random exploration\n");
        #endif

        ant.switchTo(RandomExploration{}, world);
        return;
    }
    this->explorePath.erase(this->explorePath.begin());
    this->currentStep = 0;
}

void DeterminedExploration::onChangeFrom(Ant &ant, AntWorld *world) {}

void DeterminedExploration::onTick(Ant &ant, AntWorld *world) {
    // if sees food
    auto food = ant.foodScan(world->foodMap);
    if (!food.empty()) {
#ifdef DEBUG_MOVEMENT
        printf("found food; switch to FoundFood state\n");
#endif
        ant.switchTo(FoundFood{}, world);
        return;
    }

    // if detects a food pheromone
    std::vector<Coord> foodPheromone =
        ant.pheromoneScan(world->pheromoneMap, PheromoneType::Food);
    if (!foodPheromone.empty()) {
#ifdef DEBUG_MOVEMENT
        printf("found food pheromone; switch to FollowTraill state\n");
#endif
        ant.switchTo(FollowingPheromoneTrail{}, world);
        return;
    }

    // if path ends or cannot move
    auto before = ant.position;
    if (before == ant.exploreDirection ||
        before == ant.move(world->terrainMap,
                           this->explorePath[this->currentStep++],
                           world->foodMap)) {
#ifdef DEBUG_MOVEMENT
        printf("can't continue, get new explore direction\n");
#endif
        // compute new general direction to explore
        ant.exploreDirection =
            world->getNewExploreDirection(ant.exploreDirection);

        // compute new path to new direction
        this->explorePath =
            shortestPath(world->terrainMap, ant.position, ant.exploreDirection);
        if (this->explorePath.empty()) {
            #ifdef DEBUG_MOVEMENT
                printf("shortest path returned empty list, switching to random exploration\n");
            #endif

            ant.switchTo(RandomExploration{}, world);
            return;
        }

        this->explorePath.erase(this->explorePath.begin());
        this->currentStep = 0;
    }
#ifdef DEBUG_MOVEMENT
    else {
        printf("ant moved from (%d, %d) to (%d, %d)\n", before.first,
               before.second, ant.position.first, ant.position.second);
    }
#endif

    /*
       TODO: random exploration here
    */
}

void RandomExploration::onChangeTo(Ant &ant, AntWorld *world) {}
void RandomExploration::onChangeFrom(Ant &ant, AntWorld *world) {}
void RandomExploration::onTick(Ant &ant, AntWorld *world) {}
