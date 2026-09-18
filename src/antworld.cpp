#include "../include/antworld.h"
#include <vector>
#include <random>
#include <algorithm>
#include <iostream>

//
// Created by dusan on 9/4/26.
//

void FoundFood::onChangeTo() {}
void FoundFood::onChangeFrom() {}
void FoundFood::onTick(Ant &ant, AntWorld* world) {}

void ReturningToHub::onChangeTo() {}
void ReturningToHub::onChangeFrom() {}
void ReturningToHub::onTick(Ant &ant, AntWorld* world) {}

void FollowingPheromoneTrail::onChangeTo() {}
void FollowingPheromoneTrail::onChangeFrom() {}
void FollowingPheromoneTrail::onTick(Ant &ant, AntWorld* world) {}

void DeterminedExploration::onChangeTo() {}
void DeterminedExploration::onChangeFrom() {}
void DeterminedExploration::onTick(Ant &ant, AntWorld* world) {}

void RandomExploration::onChangeTo() {}
void RandomExploration::onChangeFrom() {}
void RandomExploration::onTick(Ant &ant, AntWorld* world) {}

Ant::Ant(int initEnergy, Coord homeCoordinates) {
    // assign initial energy
    this->energy = initEnergy;

    // assign positions
    this->position = homeCoordinates;
    this->homeCoord = homeCoordinates;
}

AntWorld::AntWorld(uint32_t seed, int mapSize_x, int mapSize_y, int antCount) : rng(seed) {
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
        int initialEnergy = std::uniform_int_distribution<int>(int(mapSize_x * mapSize_y * 0.2),
                                                               int(mapSize_x * mapSize_y * 0.4))(rng);
        std::cout << initialEnergy << std::endl;
        this->ants.emplace_back(initialEnergy, this->homeCoordinates);

    }

    this->score = 0;
}

void Ant::switchTo(AntState nextState) {
    if (nextState.index() == state.index()) {
        return;
    }

    std::visit([](auto& current) {
        current.onChangeFrom();
    }, state);

    state = nextState;

    std::visit([](auto& current) {
        current.onChangeTo();
    }, state);
}

bool AntWorld::worldStep() {
    // Performs anything that needs to be done before the ants update
    this->beforeAntUpdate();

    // Performs all ant actions
    for (Ant& ant : ants) {
        std::visit([&ant, this](auto& current) {
            current.onTick(ant, this);
        }, ant.state);
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
