#include <antworld.h>

void ReturningToHub::onChangeTo(Ant& ant, AntWorld* world) {
    this->path = shortestPath(world->terrainMap, ant.position, world->homeCoordinates);
}

void ReturningToHub::onChangeFrom(Ant& ant, AntWorld* world) {}

void ReturningToHub::onTick(Ant &ant, AntWorld* world) {
    if (this->pheromoneTrail) {
        ant.dropPheromone(world->pheromoneMap, PheromoneType::Food);
    } else {
        std::vector<Coord> foods = ant.foodScan(world->foodMap);
        std::vector<Ant*> ants = ant.antScan(*world);

        Coord foodChoice = ant.foodTarget(ants, foods);

        if (foodChoice.first != -1 && foodChoice.second != -1) {
            this->pheromoneTrail = true;
        }
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