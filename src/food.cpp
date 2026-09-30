#include <antworld.h>

void ReturningToHub::onChangeTo(Ant& ant, AntWorld* world) {
    this->path = shortestPath(world->terrainMap, ant.position, ant.homeCoord);
    this->path.erase(this->path.begin());
}

void ReturningToHub::onChangeFrom(Ant& ant, AntWorld* world) {}

void ReturningToHub::onTick(Ant& ant, AntWorld* world) {
    if (this->pheromoneTrail) {
        ant.dropPheromone(world->pheromoneMap, PheromoneType::Food);
    } else {
        /* ant should drop food pheromone on sight of any other food, not just
         * the ones it is closest to */
        std::vector<Coord> foods = ant.foodScan(world->foodMap);
        if (!foods.empty()) {
            this->pheromoneTrail = true;
        }
    }

    auto before = ant.position;
    if (before == ant.homeCoord) {
#ifdef DEBUG_STATE_TRANSITION
        logStateTransition(ant.antID, "ReturningToHub", "DeterminedExploration",
                           "reached hub");
#endif
        ant.switchTo(DeterminedExploration{}, world);
        return;
    } else if (before == ant.move(world->terrainMap,
                                  this->path[this->currentStep++],
                                  world->foodMap)) {
#ifdef DEBUG_STATE_TRANSITION
        logStateTransition(ant.antID, "ReturningToHub", "Combust", "no more legal move");
#endif
        ant.combust();
        return;
    }
}

void FoundFood::onChangeTo(Ant &ant, AntWorld *world) {
    if (ant.position == this->food) {
        ant.pickupFood(world->foodMap);
    } else {
        this->path = shortestPath(world->terrainMap, ant.position, this->food);
        this->path.erase(this->path.begin());
    }
}

void FoundFood::onChangeFrom(Ant& ant, AntWorld* world) {
    // push food position that ant can't reach into
    if (ant.position != this->food) {
        ant.noReachFood.push_back(this->food);
    }
}

void FoundFood::onTick(Ant& ant, AntWorld* world) {
    auto before = ant.position;
    if (before == this->food) {
        if (ant.carryingFood) {
#ifdef DEBUG_STATE_TRANSITION
            logStateTransition(ant.antID, "FoundFood", "ReturningToHub", "reached food");
#endif
            ant.switchTo(ReturningToHub{}, world);
            return;
        } else {
#ifdef DEBUG_STATE_TRANSITION
            logStateTransition(ant.antID, "FoundFood", "DeterminedExploration", "food taken");
#endif
            ant.switchTo(DeterminedExploration{}, world);
            return;
        }
    } else if (before == ant.move(world->terrainMap,
                                  this->path[this->currentStep++],
                                  world->foodMap)) {
#ifdef DEBUG_STATE_TRANSITION
        logStateTransition(ant.antID, "FoundFood", "DeterminedExploration",
                           "out of energy for food at (%d, %d)",
                           this->food.first, this->food.second);
#endif
        ant.switchTo(DeterminedExploration{}, world);
        return;
    }

    ant.dropPheromone(world->pheromoneMap, PheromoneType::Trail, 2);
}
