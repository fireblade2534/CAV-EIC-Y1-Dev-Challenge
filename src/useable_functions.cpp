//
// Created by dusan on 9/15/26.
//

#include "../include/antworld.h"

/** @brief Checks all squares within foodRadius blocks of itself.
 *
 * @param foodMap the food layer of the world map
 *
 * @return vector of coordinates of locations in that range that have food
 */
std::vector<Coord> Ant::foodScan(MapTemplate &foodMap) {
    std::vector<Coord> foodLocations = {};
    for (int i = this->position.first - this->foodRadius; i <= this->position.first + this->foodRadius; ++i) {
        for (int j = this->position.second - this->foodRadius; j <= this->position.second + this->foodRadius; ++j) {
            if (i < 0 || i >= foodMap.size() || j < 0 || j >= foodMap[0].size()) {
                continue;
            } else {
                if (foodMap[i][j] == 1) {
                    foodLocations.emplace_back(i, j);
                }
            }
        }
    }
    return foodLocations;
}

/** @brief Checks all squares within pheromoneRadius blocks of itself.
 *
 * @param pheromoneMap the pheromone layer of the world map
 *
 * @return vector of coordinates of locations in that range that have a pheromone marker
 */
std::vector<Coord> Ant::pheromoneScan(PheromoneTemplate &pheromoneMap, PheromoneType type) {
    std::vector<Coord> pheromoneLocations = {};
    for (int i = this->position.first - this->pheromoneRadius; i <= this->position.first + this->pheromoneRadius; ++i) {
        for (int j = this->position.second - this->pheromoneRadius; j <= this->position.second + this->pheromoneRadius;
             ++j) {
            if (i < 0 || i >= pheromoneMap.size() || j < 0 || j >= pheromoneMap[0].size()) {
                continue;
            } else {
                switch (type) {
                case PheromoneType::Trail:
                    if (pheromoneMap[i][j].first != 0)
                        pheromoneLocations.emplace_back(i, j);
                    break;
                case PheromoneType::Food:
                    if (pheromoneMap[i][j].second != 0)
                        pheromoneLocations.emplace_back(i, j);
                    break;
                }
            }
        }
    }
    return pheromoneLocations;
}

/** @brief This function moves the ant to the provided step.
 * The step can only be one unit in the cardinal directions. The ant will step if it has the energy to do so. If food exists at the step, it will pick it up.
 *
 * @param terrainMap terrain layer of the world map
 * @param step coordinates of the step
 * @param foodMap food layer of the world map
 *
 * @return Coordinates of final ant position. Can be used to double check it's final position
 */
Coord Ant::move(MapTemplate &terrainMap, Coord step, MapTemplate &foodMap) {
    if (step.first >= terrainMap.size() || step.second >= terrainMap[0].size()) {
        printf("Illegal move: attempted to move to %d, %d in %d, %d grid space", step.first, step.second,
               terrainMap.size(), terrainMap[0].size());
        return this->position;
    }

    int cost = getMoveCost(terrainMap, this->position, step);

    if (cost == INFINITY) {
        printf("Illegal move: attempted to move to %d, %d which violates one tile per step", step.first, step.second);
        return this->position;
    }

    if (cost > energy) {
        printf("Illegal move: attempted to move to %d, %d which would consume %d energy when the ant has %d energy", step.first, step.second, cost, energy);
        return this->position;
    }

    this->position = step;
    energy -= cost;

    if (foodMap[this->position.first][this->position.second] == 1 && !this->carryingFood) {
        foodMap[this->position.first][this->position.second] = 0;
        this->carryingFood = true;
    }

    return this->position;
}

/** @brief removes existing pheromone and drops one at the ant's current location
 *
 * @param pheromoneMap pheromone layer of world map
 */
void Ant::dropPheromone(PheromoneTemplate &pheromoneMap, PheromoneType type) {
    if (this->pheromoneDropped) {
        erasePheromone(pheromoneMap);
    }

    std::pair<int, int> drop;

    switch (type) {
    case PheromoneType::Trail:
        drop.first = 1;
        drop.second = 0;
        break;
    case PheromoneType::Food:
        drop.first = 0;
        drop.second = 1;
        break;
    }

    pheromoneMap[this->position.first][this->position.second] = drop;
    this->pheromonePosition = this->position;
    this->pheromoneDropped = true;
}

/** @brief removes ant's existing pheromone
 *
* @param pheromoneMap pheromone layer of world map
 */
void Ant::erasePheromone(PheromoneTemplate &pheromoneMap) {
    if (this->pheromoneDropped) {
        pheromoneMap[this->pheromonePosition.first][this->pheromonePosition.second] = std::pair{0, 0};
        this->pheromonePosition = Coord(-1, -1);
        this->pheromoneDropped = false;
    }
}