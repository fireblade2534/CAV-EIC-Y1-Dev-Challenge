#include <antworld.h>

void DeterminedExploration::setExploreDirection(Coord direction, Ant &ant,
                                                AntWorld *world) {
    if (ant.exploreDirection != direction) {
        ant.exploreDirection = direction;
    }

    this->explorePath =
        shortestPath(world->terrainMap, ant.position, ant.exploreDirection);

    /* path can never be empty because of starting position. if path only has 1
     * element, onTick will change it to explore a different direction because
     * that means the ant has already reach its destination. */
    this->explorePath.erase(this->explorePath.begin());
    this->currentStep = 0;
}

void DeterminedExploration::onChangeTo(Ant &ant, AntWorld *world) {
    if (ant.position == ant.exploreDirection) {
        ant.exploreDirection =
            world->getNewExploreDirection(ant.exploreDirection);
    }

    this->setExploreDirection(ant.exploreDirection, ant, world);
    this->explore = std::bernoulli_distribution{ant.epsilon};

    // skip first reward calculation if not transiting from random exploration
    if (ant.actionIndex != 1) {
        ant.actionIndex = -1;
    }
}

/* @brief: This function decides between whether the ant should do determined
 * exploration or random exploration. The action is chosen based on an epsilon,
 * and the reward for that action is calculated using the amount of food and
 * pheromone found after performing that action. This is based on the
 * epsilon-greedy apprach. This approach yielded the best result in my testing.
 */
int DeterminedExploration::chooseAction(Ant &ant, AntWorld *world) {
    /* the ant will stick to random exploration for a certain amount of tick.
     * this yieleded the best result from simulation */
    int actionIndex;

    if (ant.randomActionCount > 0 || this->explore(world->rng) == 1) {
        if (ant.randomActionCount > 0) {
            ant.randomActionCount -= 1;
        } else {
            ant.randomActionCount = ant.stickiness;
        }

        std::uniform_int_distribution<int> dist(0, ant.actionValueTable.size() - 1);
        actionIndex = dist(world->rng);

#ifdef DEBUG_EXPLORATION
        printf("explore randomly.\n");
#endif
    }
    else {
        // exploit the best move
        auto it = std::max_element(ant.actionValueTable.begin(),
                                   ant.actionValueTable.end());

        actionIndex = std::distance(ant.actionValueTable.begin(), it);
        ant.optimalActions.push_back(actionIndex);

#ifdef DEBUG_EXPLORATION
        printf("exploit best move.\n");
#endif
    }

    return actionIndex;
}

void DeterminedExploration::onChangeFrom(Ant &ant, AntWorld *world) {
    ant.actionIndex = -1;
}

void DeterminedExploration::onTick(Ant &ant, AntWorld *world) {
    // if sees food
    std::vector<Coord> foods = ant.foodScan(world->foodMap);
    std::vector<Coord> foodPheromones =
        ant.pheromoneScan(world->pheromoneMap, PheromoneType::Food);

    // calculate reward and update value table if ant actually moved
    if (ant.actionIndex != -1) {
        int reward = world->foodScore * foods.size() +
                     world->pheromoneScore * foodPheromones.size();

        int k = ant.actionCountTable[ant.actionIndex];

        // average reward estimate
        ant.actionValueTable[ant.actionIndex] =
            (1.0 / k) * (reward - ant.actionValueTable[ant.actionIndex]);

        // update action count
        ant.actionCountTable[ant.actionIndex] = k;
    }

    /*
    if (!foods.empty()) {
#ifdef DEBUG_STATE_TRANSITION
        printf("found food; switch to FoundFood state\n");
#endif
        ant.switchTo(FoundFood{}, world);
        return;
    }
    // if detects a food pheromone
    else if (!foodPheromones.empty()) {
#ifdef DEBUG_STATE_TRANSITION
        printf("found food pheromone; switch to FollowTraill state\n");
#endif
        ant.switchTo(FollowingPheromoneTrail{}, world);
        return;
    }
    */

    /* choose whether to transition to random or keep on doing determined
     * exploration. */
    ant.actionIndex = this->chooseAction(ant, world);
    if (ant.actionIndex == 1) {
        ant.switchTo(RandomExploration{}, world);
        return;
    } else {
        // recalculate most optimal path to follow direction
        this->setExploreDirection(ant.exploreDirection, ant, world);
    }

    // if path ends or cannot move
    auto before = ant.position;
    if (before == ant.exploreDirection ||
        before == ant.move(world->terrainMap,
                           this->explorePath[this->currentStep++],
                           world->foodMap)) {
#ifdef DEBUG_MOVEMENT
        printf("can't continue, get new explore direction\n");
#endif
        printf("can't continue in given direction\n");
        // compute new general direction to explore
        this->setExploreDirection(
            world->getNewExploreDirection(ant.exploreDirection), ant, world);

        // skip reward calculation
        ant.actionIndex = -1;
    }
#ifdef DEBUG_MOVEMENT
    else {
        printf("ant moved from (%d, %d) to (%d, %d)\n", before.first,
               before.second, ant.position.first, ant.position.second);
    }
#endif
}

void RandomExploration::onChangeTo(Ant &ant, AntWorld *world) {}
void RandomExploration::onChangeFrom(Ant &ant, AntWorld *world) {
    ant.actionIndex = 1;
}

void RandomExploration::onTick(Ant &ant, AntWorld *world) {
    int moveDir = rand() % 4;
    Coord step = {ant.position.first + this->dr[moveDir],
                  ant.position.second + this->dc[moveDir]};

    // bounce the other direction if out of bounds
    if (step.first < 0) {
        step.first += 2;
    } else if ((size_t)step.first >= world->terrainMap.size()) {
        step.first -= 2;
    }

    if (step.second < 0) {
        step.second += 2;
    } else if ((size_t)step.second >= world->terrainMap[0].size()) {
        step.second -= 2;
    }

    // if ant fails, try all other directions
    if (ant.position == ant.move(world->terrainMap, step, world->foodMap)) {
        if (ant.energy > 0) {
            for (int i = 0; i < 4; i++) {
                if (ant.position.first + this->dr[i] < 0 ||
                    (size_t)ant.position.first + this->dr[i] >=
                        world->terrainMap.size() ||
                    ant.position.second + this->dc[i] < 0 ||
                    (size_t)ant.position.second + this->dc[i] >=
                        world->terrainMap[0].size()) {
                    continue;
                }

                step = {ant.position.first + this->dr[i],
                        ant.position.second + this->dc[i]};

                if (ant.position !=
                    ant.move(world->terrainMap, step, world->foodMap)) {
                    break;
                }
            }
        }
    }

    /* regardless of what happens, always transition back to
     * DeterminedExploration for reward update, which includes checking for food
     * and pheromone and transitioning to the appropriate state */
    ant.switchTo(DeterminedExploration{}, world);
}
