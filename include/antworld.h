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
    std::vector<Coord> path;
    int currentStep = 0;
    Coord food;
    
    void onChangeTo(Ant& ant, AntWorld* world);
    void onChangeFrom(Ant& ant, AntWorld* world);
    void onTick(Ant& ant, AntWorld* world);
};

struct ReturningToHub {
    std::vector<Coord> path;
    int currentStep = 0;
    bool pheromoneTrail = false;

    void onChangeTo(Ant& ant, AntWorld* world);
    void onChangeFrom(Ant& ant, AntWorld* world);
    void onTick(Ant& ant, AntWorld* world);
};

struct FollowingPheromoneTrail {
    Coord pheromoneTarget = {-1, -1};
    std::vector<Coord> explorePath;
    int currentStep = -1;
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

#ifdef DEBUG_STATE_TRANSITION
#include <stdarg.h>
inline void logStateTransition(std::string from, std::string to,
                               const char* reason, ...) {
    va_list args;
    va_start(args, reason);
    printf("Transition: %s => %s | Reason: ", from.c_str(), to.c_str());
    vfprintf(stdout, reason, args);
    va_end(args);
    printf("\n");
}
#endif

class Ant {
public:
    Ant(int initEnergy, Coord homeCoordinates, double epsilon = 0.2, int stickiness = 8);

    std::vector<Coord> foodScan(MapTemplate &foodMap);
    std::vector<Coord> pheromoneScan(PheromoneTemplate &pheromoneMap, PheromoneType type);
    std::vector<Ant*> antScan(AntWorld &antWorld);

    Coord foodTarget(std::vector<Ant*> ants, std::vector<Coord> foods);

    Coord move(MapTemplate &terrainMap, Coord step, MapTemplate &foodMap);

    void pickupFood(MapTemplate &foodMap);

    void switchTo(AntState nextState, AntWorld* world);

    void dropPheromone(PheromoneTemplate &pheromoneMap, PheromoneType type, int strength = 10);
    void erasePheromone(PheromoneTemplate &pheromoneMap, PheromoneType type);

    // self-destruct option since ant cannot move anymore
    void combust();

    bool tryLegalMove(MapTemplate& terrainMap, MapTemplate& foodMap);

    int energy{0};

    Coord homeCoord = Coord(-1, -1);
    Coord position = Coord(-1, -1);
    Coord exploreDirection = Coord(-1, -1);

    int foodRadius{3};
    int pheromoneRadius{5};
    int antRadius{5};
    bool carryingFood{false};
    AntState state{DeterminedExploration {}};

    // foods that are out of reach due to low energy
    std::vector<Coord> noReachFood;

    // exploration related fields
    double epsilon;
    int stickiness;
    std::vector<double> actionValueTable = {0.0, 0.0};
    std::vector<int> actionCountTable = {0, 0};
    std::vector<int> optimalActions;
    int randomActionCount = 0;

    /* actionIndex can either be -1 for when ant does not move, 0 for when ant
     * moves according to determined exploration and 1 when ant moves according
     * to random exploration */
    int actionIndex = 0;

    /* class members */
    constexpr static int dr[4] = {1, -1, 0, 0};
    constexpr static int dc[4] = {0, 0, 1, -1};
};

class AntWorld {
public:
#ifdef DEBUG_SINGLE_ANT
#define ANTCOUNT 1
#else
#define ANTCOUNT 8
#endif
    AntWorld(uint32_t seed, int mapSize_x = 15, int mapSize_y = 15, int antCount = ANTCOUNT);

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
