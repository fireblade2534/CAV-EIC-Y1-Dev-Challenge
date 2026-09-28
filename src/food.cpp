#include <antworld.h>


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