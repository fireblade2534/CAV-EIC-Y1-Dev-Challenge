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
    void onChangeTo(Ant& ant, AntWorld* world);
    void onChangeFrom(Ant& ant, AntWorld* world);
    void onTick(Ant& ant, AntWorld* world);
};

struct DeterminedExploration {
    std::vector<Coord> explorePath;
    int currentStep = -1;
    std::bernoulli_distribution explore;

    void setExploreDirection(Coord direction, Ant&ant, AntWorld* world);
    void onChangeTo(Ant& ant, AntWorld* world);
    void onChangeFrom(Ant& ant, AntWorld* world);
    void onTick(Ant& ant, AntWorld* world);
    int chooseAction(Ant& ant, AntWorld* world);
};

struct RandomExploration {
    int dr[4] = {1, -1, 0, 0};
    int dc[4] = {0, 0, 1, -1};

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
    Ant(int initEnergy, Coord homeCoordinates, double epsilon = 0.2, int stickiness = 8);

    std::vector<Coord> foodScan(MapTemplate &foodMap);

    std::vector<Coord> pheromoneScan(PheromoneTemplate &pheromoneMap, PheromoneType type);

    Coord move(MapTemplate &terrainMap, Coord step, MapTemplate &foodMap);

    void switchTo(AntState nextState, AntWorld* world);

    void dropPheromone(PheromoneTemplate &pheromoneMap, PheromoneType type, int strength = 10);
    void erasePheromone(PheromoneTemplate &pheromoneMap, PheromoneType type);

    int energy{0};

    Coord homeCoord = Coord(-1, -1);
    Coord position = Coord(-1, -1);
    Coord exploreDirection = Coord(-1, -1);

    int foodRadius{3};
    int pheromoneRadius{5};
    bool carryingFood{false};
    AntState state{DeterminedExploration {}};

    // exploration related fields
    double epsilon;

    /* actionIndex can either be -1 for when ant does not move, 0 for when ant
     * moves according to determined exploration and 1 when ant moves according
     * to random exploration */
    int actionIndex = 0;

    std::vector<double> actionValueTable = {0.0, 0.0};
    std::vector<int> actionCountTable = {0, 0};
    std::vector<int> optimalActions;
    int randomActionCount = 0;
    int stickiness = stickiness;
};

class AntWorld {
public:
    AntWorld(uint32_t seed, int mapSize_x = 15, int mapSize_y = 15, int antCount = 8);

    bool worldStep();

    void beforeAntUpdate();

    void afterAntUpdate();

    void updateWorld();

    bool isGameOver();

    Coord getNewExploreDirection(Coord oldDirection);

    MapTemplate terrainMap;
    MapTemplate foodMap;
    PheromoneTemplate pheromoneMap;

    std::vector<Ant> ants = {};
    std::vector<Coord> exploreDirections;
    Coord homeCoordinates = Coord(-1, -1);

    int score = 0;

    // internal exploration score system
    int foodScore = 3;
    int pheromoneScore = 2;
    std::mt19937 rng;

private:
    std::uniform_int_distribution<int> exploreDirDist{0, 7};
};


#endif //DEV_CHALLENGE_ANTWORLD_H
