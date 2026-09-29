#include <antworld.h>

void ReturningToHub::onChangeTo(Ant& ant, AntWorld* world) {
    this->path = shortestPath(world->terrainMap, ant.position, ant.homeCoord);
}

void ReturningToHub::onChangeFrom(Ant& ant, AntWorld* world) {}

void ReturningToHub::onTick(Ant& ant, AntWorld* world) {
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
    if (before == ant.move(world->terrainMap, this->path[this->currentStep++],
                           world->foodMap)) {
#ifdef DEBUG_STATE_TRANSITION
        if (before == ant.homeCoord) {
            logStateTransition("ReturningToHub", "DeterminedExploration",
                               "reached hub");
        } else {
            logStateTransition("ReturningToHub", "DeterminedExploration",
                               "out of energy");
        }
#endif
        ant.switchTo(DeterminedExploration{}, world);
        return;
    }
}

void FoundFood::onChangeTo(Ant& ant, AntWorld* world) {
    this->path = shortestPath(world->terrainMap, ant.position, this->food);
}

void FoundFood::onChangeFrom(Ant& ant, AntWorld* world) {
    // push food position that ant can't reach into
    if (ant.position != this->food) {
        ant.noReachFood.push_back(this->food);
    }
}

void FoundFood::onTick(Ant& ant, AntWorld* world) {
    auto before = ant.position;
    if (before == ant.move(world->terrainMap, this->path[this->currentStep++],
                           world->foodMap)) {
        if (ant.position == this->food) {
#ifdef DEBUG_STATE_TRANSITION
            logStateTransition("FoundFood", "ReturningToHub", "reached food");
#endif
            ant.switchTo(ReturningToHub{}, world);
            return;
        } else {
#ifdef DEBUG_STATE_TRANSITION
            logStateTransition("FoundFood", "DeterminedExploration",
                               "out of energy for food at (%d, %d)",
                               this->food.first, this->food.second);
#endif
            ant.switchTo(DeterminedExploration{}, world);
            return;
        }
    }
}
