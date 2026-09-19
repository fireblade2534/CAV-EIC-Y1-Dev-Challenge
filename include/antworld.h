#ifndef DEV_CHALLENGE_ANTWORLD_H
#define DEV_CHALLENGE_ANTWORLD_H

#include <vector>
#include <random>
#include "utility_functions.h"
#include <variant>
//
// Created by dusan on 9/4/26.
//

class Ant;
class AntWorld;

struct FoundFood {
    void onChangeTo(Ant& ant, AntWorld* world);
    void onChangeFrom(Ant& ant, AntWorld* world);
    void onTick(Ant& ant, AntWorld* world);
};

struct ReturningToHub {
    void onChangeTo(Ant& ant, AntWorld* world);
    void onChangeFrom(Ant& ant, AntWorld* world);
    void onTick(Ant& ant, AntWorld* world);
};

struct FollowingPheromoneTrail {
    Coord foodTarget = {-1, -1};

    void onChangeTo(Ant& ant, AntWorld* world);
    void onChangeFrom(Ant& ant, AntWorld* world);
    void onTick(Ant& ant, AntWorld* world);
};

struct DeterminedExploration {
    void onChangeTo(Ant& ant, AntWorld* world);
    void onChangeFrom(Ant& ant, AntWorld* world);
    void onTick(Ant& ant, AntWorld* world);
};

struct RandomExploration {
    void onChangeTo(Ant &ant, AntWorld *world);
    void onChangeFrom(Ant& ant, AntWorld* world);
    void onTick(Ant& ant, AntWorld* world);
};

using AntState = std::variant<
    FoundFood,
    ReturningToHub,
    FollowingPheromoneTrail,
    DeterminedExploration,
    RandomExploration
>;

class Ant {
public:
    Ant(int initEnergy, Coord homeCoordinates);

    std::vector<Coord> foodScan(MapTemplate &foodMap);

    std::vector<Coord> pheromoneScan(PheromoneTemplate &pheromoneMap, PheromoneType type);

    Coord move(MapTemplate &terrainMap, Coord step, MapTemplate &foodMap);

    void switchTo(AntState nextState, AntWorld* world);

    void dropPheromone(PheromoneTemplate &pheromoneMap, PheromoneType type);

    void erasePheromone(PheromoneTemplate &pheromoneMap);

    int energy{0};

    Coord homeCoord = Coord(-1, -1);
    Coord position = Coord(-1, -1);

    int foodRadius{3};
    int pheromoneRadius{5};
    bool pheromoneDropped{false};
    Coord pheromonePosition = Coord(-1, -1);
    bool carryingFood{false};
    AntState state{DeterminedExploration {}};
};

class AntWorld {
public:
    AntWorld(uint32_t seed, int mapSize_x = 15, int mapSize_y = 15, int antCount = 8);

    bool worldStep();

    void beforeAntUpdate();

    void afterAntUpdate();

    void updateWorld();

    bool isGameOver();

    MapTemplate terrainMap;
    MapTemplate foodMap;
    PheromoneTemplate pheromoneMap;

    std::vector<Ant> ants = {};
    Coord homeCoordinates = Coord(-1, -1);

    int score = 0;

private:
    std::mt19937 rng;
};


#endif //DEV_CHALLENGE_ANTWORLD_H
