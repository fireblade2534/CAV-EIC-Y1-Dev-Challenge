#include "../include/antworld.h"
#include <iostream>
#include <random>
#include <vector>

//
// Created by dusan on 9/4/26.
//

Ant::Ant(int ID, int initEnergy, Coord homeCoordinates, double epsilon, int stickiness) {
    // assign initial energy
    this->energy = initEnergy;

    this -> antID = ID;

    // assign positions
    this->position = homeCoordinates;
    this->homeCoord = homeCoordinates;
    this->epsilon = epsilon;
    this->stickiness = stickiness;
}

AntWorld::AntWorld(uint32_t seed, int mapSize_x, int mapSize_y, int antCount)
    : rng(seed) {
    // Generate the various world map layers
    this->terrainMap = generateWorldMap(mapSize_x, mapSize_y, this->rng);
    // come back to this
    this->foodCount = int(mapSize_x * mapSize_y * 0.4);
    this->foodMap = spreadFood(mapSize_x, mapSize_y, this->foodCount, this->rng);
    this->pheromoneMap = PheromoneTemplate(
        mapSize_x, std::vector<std::pair<int, int>>(mapSize_y, {0, 0}));
    this->score = 0;

    // Randomly generating home coordinates

    std::uniform_int_distribution<int> rowDist(0, mapSize_x - 1);
    std::uniform_int_distribution<int> colDist(0, mapSize_y - 1);
    this->homeCoordinates = {rowDist(rng), colDist(rng)};

#ifdef DEBUG_STATE_TRANSITION
    printf("HOME COORD: (%d, %d)\n", this->homeCoordinates.first,
           this->homeCoordinates.second);
#endif

    // initialize all the ants
    for (int i = 0; i < antCount; ++i) {
        // and initial energy for each ant
        int initialEnergy = std::uniform_int_distribution<int>(
            int(mapSize_x * mapSize_y * 0.2),
            int(mapSize_x * mapSize_y * 0.4))(rng);
        std::cout << initialEnergy << std::endl;
        this->ants.emplace_back(i, initialEnergy, this->homeCoordinates);
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

    /* assign foods that are close to base to avoid early collision where all
     * distances are the same */
    auto nearbyFoods = ants[0].foodScan(foodMap);
    for (int i = 0, currentAntIndex = 0;
         i < std::min((int)nearbyFoods.size(), ANTCOUNT); i++, currentAntIndex++) {
        auto &ant = ants[currentAntIndex];
        auto &food = nearbyFoods[i];
        ant.switchTo(FoundFood{.path = {}, .food = food}, this);
    }
    
#ifdef DEBUG_TOTAL_ENERGY
    /* calculate energy for all food sources as a debug parameter */
    std::vector<Coord> allFoodLoc;
    for (int i  = 0; (size_t)i < foodMap.size(); i++) {
        for (int j = 0; (size_t)j < foodMap[0].size(); j++) {
            if (foodMap[i][j] == 1) {
                allFoodLoc.push_back(Coord{i, j});
            }
        }
    }

    int totalCost = 0;
    int totalAntEnergy = 0;
    for (auto & loc : allFoodLoc) {
        auto steps = shortestPath(terrainMap, homeCoordinates, loc);
        steps.erase(steps.begin());

        int energy = 0;
        Coord position = homeCoordinates;

        for (auto step : steps) {
            int cost = position == step
                           ? 0
                           : getMoveCost(terrainMap, position, step);
            position = step;
            energy += cost;
        }

        totalCost += energy;
    }

    for (auto& ant : ants) {
        totalAntEnergy += ant.energy;
    }

    printf("total cost: %d, total ant energy: %d\n", totalCost, totalAntEnergy);
#endif
}

Coord AntWorld::getNewExploreDirection(Coord oldDirection) {
    int newDir = this->exploreDirDist(this->rng);
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

bool Ant::anyLegalMove(MapTemplate &terrainMap, MapTemplate &foodMap) {
    for (int i = 0; i < 4; i++) {
        if (this->position.first + Ant::dr[i] < 0 ||
            (size_t)this->position.first + Ant::dr[i] >= terrainMap.size() ||
            this->position.second + Ant::dc[i] < 0 ||
            (size_t)this->position.second + Ant::dc[i] >=
                terrainMap[0].size()) {
            continue;
        }

        Coord step{this->position.first + Ant::dr[i], this->position.second + Ant::dc[i]};

        auto before = this->position;
        if (before != this->move(terrainMap, step, foodMap)) {
            return true;
        }
    }

    return false;
}

void Ant::combust() {
    this->energy = 0;
}

bool AntWorld::worldStep() {
    // Performs anything that needs to be done before the ants update
    this->beforeAntUpdate();

    // Performs all before tick actions
    for (Ant &ant : ants) {
        std::visit([&ant, this](auto &current) { current.beforeTick(ant, this); },
                   ant.state);
    } 

    // Performs all ant actions
    for (Ant &ant : ants) {
        std::visit([&ant, this](auto &current) { current.onTick(ant, this); },
                   ant.state);
    }

    // Performs anything that needs to be done after the ants update
    this->afterAntUpdate();

    // Checks game over state
    return this->isGameOver();
}

void AntWorld::beforeAntUpdate() {
    updatePheromones(this->pheromoneMap);
}

void AntWorld::afterAntUpdate() {
    for (auto it = this->ants.begin(); it != this->ants.end();) {
        // Check if any ants are at the home coordinate, with food
        // if so, increase point count
        if (it->position == this->homeCoordinates && it->carryingFood) {
            this->score++;
            it->carryingFood = false;
        }

        // If ant is out of energy and carrying food, drop food at last position
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
    // If there are no remaining ants, game over
    if (this->ants.empty()) {
        return true;
    }

    // If there is no remaining food, congrats, game over
    if (not hasFood(foodMap)) {
        return true;
    }

    return false;
}
