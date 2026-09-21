#include <antworld.h>

void DeterminedExploration::onChangeTo(Ant &ant, AntWorld *world) {
    if (ant.position == ant.exploreDirection) {
        ant.exploreDirection =
            world->getNewExploreDirection(ant.exploreDirection);
    }

    this->explorePath =
        shortestPath(world->terrainMap, ant.position, ant.exploreDirection);
    this->explorePath.erase(this->explorePath.begin());
    this->currentStep = 0;
}

void DeterminedExploration::onChangeFrom(Ant &ant, AntWorld *world) {}

void DeterminedExploration::onTick(Ant &ant, AntWorld *world) {
    // if sees food
    auto food = ant.foodScan(world->foodMap);
    if (!food.empty()) {
        ant.switchTo(FoundFood{}, world);
        return;
    }

    // if detects a food pheromone
    std::vector<Coord> foodPheromone =
        ant.pheromoneScan(world->pheromoneMap, PheromoneType::Food);
    if (!foodPheromone.empty()) {
        ant.switchTo(FollowingPheromoneTrail{}, world);
        return;
    }

    // if path ends or cannot move
    auto before = ant.position;
    if (before == ant.exploreDirection ||
        before == ant.move(world->terrainMap,
                           this->explorePath[this->currentStep++],
                           world->foodMap)) {
        // compute new general direction to explore
        ant.exploreDirection =
            world->getNewExploreDirection(ant.exploreDirection);

        // compute new path to new direction
        this->explorePath =
            shortestPath(world->terrainMap, ant.position, ant.exploreDirection);
        this->explorePath.erase(this->explorePath.begin());
        this->currentStep = 0;
    }

    /*
       TODO: random exploration here
    */
}

void RandomExploration::onChangeTo(Ant &ant, AntWorld *world) {}
void RandomExploration::onChangeFrom(Ant &ant, AntWorld *world) {}
void RandomExploration::onTick(Ant &ant, AntWorld *world) {}
