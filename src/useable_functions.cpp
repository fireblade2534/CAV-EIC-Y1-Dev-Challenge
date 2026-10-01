//
// Created by dusan on 9/15/26.
//

#include "../include/antworld.h"

/** @brief Checks all squares within foodRadius blocks of itself.
 *
 * @param foodMap The food layer of the world map
 * @param filterNoReach Filters out food that the ant cannot reach
 *
 * @return Vector of coordinates of locations in that range that have food
 */
std::vector<Coord> Ant::foodScan(MapTemplate &foodMap, bool filterNoReach) {
    std::vector<Coord> foodLocations = {};
    for (int i = this->position.first - this->foodRadius; i <= this->position.first + this->foodRadius; ++i) {
        for (int j = this->position.second - this->foodRadius; j <= this->position.second + this->foodRadius; ++j) {
            if (i < 0 || i >= foodMap.size() || j < 0 || j >= foodMap[0].size()) {
                continue;
            } else if (foodMap[i][j] == 1) {
                if (filterNoReach) {
                    if (std::find(this->noReachFood.begin(), this->noReachFood.end(), Coord {i, j}) != this->noReachFood.end()) {
                        continue;
                    }
                }

                foodLocations.emplace_back(i, j);
            }
        }
    }
    return foodLocations;
}

/** @brief Checks all squares within pheromoneRadius blocks of itself.
 *
 * @param pheromoneMap The pheromone layer of the world map
 *
 * @return Vector of coordinates of locations in that range that have a pheromone marker
 */
std::vector<Coord> Ant::pheromoneScan(PheromoneTemplate &pheromoneMap, PheromoneType type) {
    std::vector<Coord> pheromoneLocation;

    for (int i = this->position.first - this->pheromoneRadius; i <= this->position.first + this->pheromoneRadius; ++i) {
        for (int j = this->position.second - this->pheromoneRadius; j <= this->position.second + this->pheromoneRadius;
             ++j) {
            if (i < 0 || i >= pheromoneMap.size() || j < 0 || j >= pheromoneMap[0].size()) {
                continue;
            } else {
                switch (type) {
                case PheromoneType::Position:
                    if (pheromoneMap[i][j].first != 0)
                        pheromoneLocation.emplace_back(i, j);
                    break;
                case PheromoneType::Food:
                    if (pheromoneMap[i][j].second != 0)
                        pheromoneLocation.emplace_back(i, j);
                    break;
                }
            }
        }
    }

    return pheromoneLocation;
}

/** @brief Checks all ants within pheromoneRadius blocks of itself.
 *
 * @param antWorld the world
 *
 * @return vector of ants in that range
 */
std::vector<Ant*> Ant::antScan(AntWorld &antWorld) {
    std::vector<Ant*> ants = {};
    for (Ant& ant : antWorld.ants) {
        if (&ant == this) {
            continue;
        }
            
        if (std::abs(ant.position.first - this->position.first) <= antRadius && std::abs(ant.position.second - this->position.second) <= antRadius) {
            ants.push_back(&ant);
        }
    }

    return ants;
}

/** @brief Chooses which food to go to
 *
 * @param foods The vector of foods in range
 * @param positions The vector of ants in range
 * @param rng The rng generator
 *
 * @return The target food for the ant. Will be -1, -1 if the ant shouldn't go to a food
 * 
 */
Coord Ant::foodTarget(std::vector<Coord> foods, std::vector<Coord> positions, std::mt19937& rng) {
    if (foods.empty()) {
        return {-1, -1};
    }

    std::sort(foods.begin(), foods.end(),
        [this](const Coord& a, const Coord& b) {
            int distanceA = getManhattanDistance(this->position, a);
            int distanceB = getManhattanDistance(this->position, b);

            return distanceA < distanceB;
        }
    );

    for (const Coord& food : foods) {
        int selfDistance = getManhattanDistance(this->position, food);

        bool selfClosest = true;
        std::vector<Coord> antWithSameDist;

        for (Coord otherAnt : positions) {
            if (otherAnt.first == this->position.first && otherAnt.second == this->position.second) {
                continue;
            }

            // Outside of the other ant's detection radius
            if (std::abs(otherAnt.first - food.first) >
                    this->foodRadius ||
                std::abs(otherAnt.second - food.second) >
                    this->foodRadius) {
                continue;
            }

            int theirDistance = getManhattanDistance(otherAnt, food);

            if (theirDistance < selfDistance) {
                selfClosest = false;
                break;
            } else if (selfDistance == theirDistance) {
                antWithSameDist.push_back(otherAnt);
            }
        }

        if (selfClosest) {
            if (antWithSameDist.empty()) {
                return food;
            }

            /* (1 / n) chance of going for the food, with n being the number
                * of ant at the same distance */
            std::uniform_int_distribution<int> dist(
                1,
                static_cast<int>(antWithSameDist.size()) + 1
            );

            int willGo = dist(rng);
            if (willGo == 1) {
                return food;
            }
        }
    }

    return {-1, -1};
}

void Ant::pickupFood(MapTemplate &foodMap) {
    if (foodMap[this->position.first][this->position.second] == 1 && !this->carryingFood) {
        foodMap[this->position.first][this->position.second] = 0;
        this->carryingFood = true;
    }
}

/** @brief This function moves the ant to the provided step.
 * The step can only be one unit in the cardinal directions. The ant will step if it has the energy to do so. If food exists at the step, it will pick it up. If ant is already at location, return ant's current locatino with no cost.
 *
 * @param terrainMap terrain layer of the world map
 * @param step coordinates of the step
 * @param foodMap food layer of the world map
 *
 * @return Coordinates of final ant position. Can be used to double check it's final position
 */
Coord Ant::move(MapTemplate &terrainMap, Coord step, MapTemplate &foodMap) {
    if (step.first >= (int)terrainMap.size() ||
        step.second >= (int)terrainMap[0].size()) {
        printf("Illegal move: attempted to move to (%d, %d) in a (%d, %d) "
               "grid space\n",
               step.first, step.second, (int)terrainMap.size(),
               (int)terrainMap[0].size());
        return this->position;
    }

    int cost = this->position == step
                   ? 0
                   : getMoveCost(terrainMap, this->position, step);

    if (cost == INFINITY) {
        #ifdef DEBUG_MOVEMENT
        printf("Illegal move: attempted to move to %d, %d which violates one tile per step\n", step.first, step.second);
        #endif
        return this->position;
    } else if (cost > this->energy) {
        #ifdef DEBUG_MOVEMENT
        printf("Illegal move: attempted to move to %d, %d which would consume %d energy when the ant has %d energy\n", step.first, step.second, cost, this->energy);
        #endif
        return this->position;
    }
    #ifdef DEBUG_MOVEMENT
    printf(
        "Ant %d moving from (%d, %d) to (%d, %d) costed %d energy. Energy left: %d\n",
        this->antID, this->position.first, this->position.second, step.first, step.second,
        cost, this->energy - cost);
    #endif

    this->position = step;
    this->energy -= cost;

    this->pickupFood(foodMap);

    return this->position;
}

/** @brief drops a pheromone at the ant's current location
 *
 * @param pheromoneMap pheromone layer of world map
 */
void Ant::dropPheromone(PheromoneTemplate &pheromoneMap, PheromoneType type, int strength) {


    switch (type) {
    case PheromoneType::Position:
        if (pheromoneMap[this->position.first][this->position.second].first < strength) {
            pheromoneMap[this->position.first][this->position.second].first = strength;
        }
        break;
    case PheromoneType::Food:
        if (pheromoneMap[this->position.first][this->position.second].second < strength) {
            pheromoneMap[this->position.first][this->position.second].second = strength;
        }
        break;
    }
}

/** @brief erase a pheromone at the ant's current location
 *
 * @param pheromoneMap pheromone layer of world map
 */
void Ant::erasePheromone(PheromoneTemplate &pheromoneMap, PheromoneType type) {


    switch (type) {
    case PheromoneType::Position:
        pheromoneMap[this->position.first][this->position.second].first = 0;
        break;
    case PheromoneType::Food:
        pheromoneMap[this->position.first][this->position.second].second = 0;
        break;
    }
}
