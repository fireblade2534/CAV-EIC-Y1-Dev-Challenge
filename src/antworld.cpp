#include "../include/antworld.h"
#include <algorithm>
#include <iostream>
#include <random>
#include <vector>

//
// Created by dusan on 9/4/26.
//

void FoundFood::onChangeTo(Ant &ant, AntWorld *world) {}
void FoundFood::onChangeFrom(Ant &ant, AntWorld *world) {}
void FoundFood::onTick(Ant &ant, AntWorld *world) {}

void ReturningToHub::onChangeTo(Ant& ant, AntWorld* world) {}
void ReturningToHub::onChangeFrom(Ant& ant, AntWorld* world) {}
void ReturningToHub::onTick(Ant &ant, AntWorld* world) {}

void FollowingPheromoneTrail::onChangeTo(Ant& ant, AntWorld* world) {}
void FollowingPheromoneTrail::onChangeFrom(Ant& ant, AntWorld* world) {}
void FollowingPheromoneTrail::onTick(Ant &ant, AntWorld* world) {}

Ant::Ant(int initEnergy, Coord homeCoordinates) {
    // assign initial energy
    this->energy = initEnergy;

    // assign positions
    this->position = homeCoordinates;
    this->homeCoord = homeCoordinates;
}

AntWorld::AntWorld(uint32_t seed, int mapSize_x, int mapSize_y, int antCount)
    : rng(seed) {
    // Generate the various world map layers
    this->terrainMap = generateWorldMap(mapSize_x, mapSize_y, this->rng);
    // come back to this
    int foodCount = int(mapSize_x * mapSize_y * 0.4);
    this->foodMap = spreadFood(mapSize_x, mapSize_y, foodCount, this->rng);
    this->pheromoneMap = PheromoneTemplate(
        mapSize_x, std::vector<std::pair<int, int>>(mapSize_y, {0, 0}));

    // Randomly generating home coordinates

    std::uniform_int_distribution<int> rowDist(0, mapSize_x - 1);
    std::uniform_int_distribution<int> colDist(0, mapSize_y - 1);
    this->homeCoordinates = {rowDist(rng), colDist(rng)};

    // initialize all the ants
    for (int i = 0; i < antCount; ++i) {
        // and initial energy for each ant
        int initialEnergy = std::uniform_int_distribution<int>(
            int(mapSize_x * mapSize_y * 0.2),
            int(mapSize_x * mapSize_y * 0.4))(rng);
        std::cout << initialEnergy << std::endl;
        this->ants.emplace_back(initialEnergy, this->homeCoordinates);
    }

    // assign general exploration direction for the ants
    const int cx = mapSize_x / 2;
    const int cy = mapSize_y / 2;

    this->exploreDirections = {{0, 0},
                               {0, cy},
                               {0, mapSize_y - 1},
                               {cx, 0},
                               {cx, mapSize_y - 1},
                               {mapSize_x - 1, 0},
                               {mapSize_x - 1, cy},
                               {mapSize_x - 1, mapSize_y - 1}};
    int currentDir = 0;
    for (auto &ant : ants) {
        ant.exploreDirection = this->exploreDirections[currentDir];
        currentDir = (currentDir + 1) % this->exploreDirections.size();
    }

    // initial random exploration setup
    for (auto &ant: ants) {
        std::visit(
            [&ant, this](auto &current) { current.onChangeTo(ant, this); },
            ant.state);
    }

    this->score = 0;
}

Coord AntWorld::getNewExploreDirection(Coord oldDirection) {
    int newDir = this->exploreDirDist(this->rng);
    printf("newDir: %d\n", newDir);

    if (this->exploreDirections[newDir] == oldDirection) {
        newDir = (newDir + 1) % this->exploreDirections.size();
    }

    return this->exploreDirections[newDir];
}

void Ant::switchTo(AntState nextState, AntWorld *world) {
    if (nextState.index() == state.index()) {
        return;
    }

    std::visit(
        [this, world](auto &current) { current.onChangeFrom(*this, world); },
        state);

    state = nextState;

    std::visit(
        [this, world](auto &current) { current.onChangeTo(*this, world); },
        state);
}

bool AntWorld::worldStep() {
    // Performs anything that needs to be done before the ants update
    this->beforeAntUpdate();

    // Performs all ant actions
    for (Ant &ant : ants) {
        std::visit([&ant, this](auto &current) { current.onTick(ant, this); },
                   ant.state);
    }

    // Performs anything that needs to be done after the ants update
    this->afterAntUpdate();

    // updates score and cleans ants
    this->updateWorld();

    // checks game over state
    return this->isGameOver();
}

void AntWorld::updateWorld() {
    for (auto it = this->ants.begin(); it != this->ants.end();) {
        // check if any ants are at the home coordinate, with food
        // if so, increase point count
        if (it->position == this->homeCoordinates && it->carryingFood) {
            this->score++;
            it->carryingFood = false;
        }
        // if ant is out of energy and carrying food, drop food at last position
        // delete the ant from the array
        if (it->energy == 0) {
            if (it->carryingFood) {
                this->foodMap[it->position.first][it->position.second] = 1;
            }
            it = this->ants.erase(it);
        } else {
            ++it;
        }
    }
}

bool AntWorld::isGameOver() {
    // if there are no remaining ants, game over
    if (this->ants.empty()) {
        return true;
    }

    // if there is no remaining food, congrats, game over
    if (not hasFood(foodMap)) {
        return true;
    }

    // else the game continues
    return false;
}
