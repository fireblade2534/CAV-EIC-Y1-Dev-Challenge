#include <antworld.h>

void ReturningToHub::onChangeTo(Ant& ant, AntWorld* world) {
    this->path = shortestPath(world->terrainMap, ant.position, world->homeCoordinates);
}

void ReturningToHub::onChangeFrom(Ant& ant, AntWorld* world) {}

void ReturningToHub::onTick(Ant &ant, AntWorld* world) {
    if (!this->pheromoneTrail) {
        std::vector<Coord> foods = ant.foodScan(world->foodMap);
        std::vector<Coord> foodPheromones = ant.pheromoneScan(world->pheromoneMap, PheromoneType::Food);
    }

    auto before = ant.position;
    if (before == ant.move(world->terrainMap, this->path[this->currentStep++], world->foodMap)) {
        ant.switchTo(DeterminedExploration{}, world);
        return;
    }
}


void FoundFood::onChangeTo(Ant &ant, AntWorld *world) {
    this->path = shortestPath(world->terrainMap, ant.position, this->food);
}

void FoundFood::onChangeFrom(Ant &ant, AntWorld *world) {}

void FoundFood::onTick(Ant &ant, AntWorld *world) {

    auto before = ant.position;
    if (before == ant.move(world->terrainMap, this->path[this->currentStep++], world->foodMap)) {
        ant.switchTo(DeterminedExploration{}, world);
        return;
    }

    if (ant.position == this->food) {
        ant.switchTo(ReturningToHub{}, world);
        return;
    }
}