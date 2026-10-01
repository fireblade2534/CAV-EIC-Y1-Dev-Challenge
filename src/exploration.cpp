#include <antworld.h>

void DeterminedExploration::setExploreDirection(Coord direction, Ant &ant,
                                                AntWorld *world) {
    if (ant.exploreDirection != direction) {
        ant.exploreDirection = direction;
    }

    this->explorePath =
        shortestPath(world->terrainMap, ant.position, ant.exploreDirection);

    /* Path can never be empty because of starting position. if path only has 1
     * element, onTick will change it to explore a different direction because
     * that means the ant has already reach its destination. */
    this->explorePath.erase(this->explorePath.begin());
    this->currentStep = 0;
}

void DeterminedExploration::onChangeTo(Ant &ant, AntWorld *world) {
    // If the ant has reached its explored direction destination it picks a new explore direction
    if (ant.position == ant.exploreDirection) {
        ant.exploreDirection =
            world->getNewExploreDirection(ant.exploreDirection);
    }

    this->setExploreDirection(ant.exploreDirection, ant, world);
    this->explore = std::bernoulli_distribution{ant.epsilon};

    // Skip first reward calculation if not transiting from random exploration
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
    /* The ant will stick to random exploration for a certain amount of ticks.
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
    } else {
        // Exploit the best move
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
    std::vector<Coord> foods = ant.foodScan(world->foodMap);
    std::vector<Ant*> ants = ant.antScan(*world);
    std::vector<Coord> foodPheromones = ant.pheromoneScan(world->pheromoneMap, PheromoneType::Food);
    std::vector<Coord> positionPheromones = ant.pheromoneScan(world->pheromoneMap, PheromoneType::Position);

    // Calculate reward and update value table if ant actually moved
    if (ant.actionIndex != -1) {
        // Reward is still based on the total food, not just valid ones
        int reward = world->foodScore * foods.size() +
                     world->pheromoneScore * foodPheromones.size();

        int k = ant.actionCountTable[ant.actionIndex];

        // average reward estimate
        ant.actionValueTable[ant.actionIndex] =
            (1.0 / k) * (reward - ant.actionValueTable[ant.actionIndex]);

        // update action count
        ant.actionCountTable[ant.actionIndex] = k;
    }

    Coord foodChoice = ant.foodTarget(ants, foods, positionPheromones, world->rng);
    if (foodChoice.first != -1 && foodChoice.second != -1) {
#ifdef DEBUG_STATE_TRANSITION
        logStateTransition(ant.antID, "DeterminedExploration", "FoundFood",
                           "found food at (%d, %d)", foodChoice.first,
                           foodChoice.second);
#endif
        ant.switchTo(FoundFood{.path = {}, .food = foodChoice}, world);
        return;
    } else if (!foodPheromones.empty()) { // If detects a food pheromone
#ifdef DEBUG_STATE_TRANSITION
        logStateTransition(ant.antID, "DeterminedExploration", "FollowingPheromoneTrail", "found pheromone trail");
#endif
        ant.switchTo(FollowingPheromoneTrail{}, world);
        return;
    }

    /* Choose whether to transition to random or keep on doing determined
     * exploration. */
    ant.actionIndex = this->chooseAction(ant, world);
    if (ant.actionIndex == 1) {
#ifdef DEBUG_STATE_TRANSITION
        logStateTransition(ant.antID, "DeterminedExploration", "RandomExploration", "chosen action");
#endif
        ant.switchTo(RandomExploration{}, world);
        return;
    } else {
        // Recalculate most optimal path to follow direction
        this->setExploreDirection(ant.exploreDirection, ant, world);
    }

    // If path ends or cannot move
    auto before = ant.position;
    if (before == ant.exploreDirection) {
#ifdef DEBUG_MOVEMENT
        printf("reach original direction, get new one\n");
#endif
        this->setExploreDirection(
            world->getNewExploreDirection(ant.exploreDirection), ant, world);
        // Skip reward calculation
        ant.actionIndex = -1;
    } else if (before == ant.move(world->terrainMap,
                                  this->explorePath[this->currentStep++],
                                  world->foodMap)) {
        // Try any move to avoid wasting tick
        bool valid = ant.anyLegalMove(world->terrainMap, world->foodMap);

        // If ant can still make moves, it only needs different direction
        if (valid) {
#ifdef DEBUG_MOVEMENT
            printf("can't continue but has energy, get new explore direction.\n");
#endif
            this->setExploreDirection(
                world->getNewExploreDirection(ant.exploreDirection), ant,
                world);
            /* Random move shouldn't affect reward calculation */
            ant.actionIndex = -1;
        } else {
#ifdef DEBUG_STATE_TRANSITION
            logStateTransition(ant.antID, "DeterminedExploration", "Combust",
                               "no more legal move");
#endif
            ant.combust();
            return;
        }
    }
#ifdef DEBUG_MOVEMENT
    else {
        printf("ant moved from (%d, %d) to (%d, %d)\n", before.first,
               before.second, ant.position.first, ant.position.second);
    }
#endif
    ant.dropPheromone(world->pheromoneMap, PheromoneType::Position, 2);
}

void RandomExploration::onChangeTo(Ant &ant, AntWorld *world) {}
void RandomExploration::onChangeFrom(Ant &ant, AntWorld *world) {
    ant.actionIndex = 1;
}

void RandomExploration::onTick(Ant &ant, AntWorld *world) {
    /* The function doesn't need any food / pheromone check because it is guaranteed that the checks have already happened (and the rewards have been updated) before state transitions to random exploration. */
    std::uniform_int_distribution<int> moveDirDist(0, 3);
    int moveDir = moveDirDist(world->rng);

    Coord step = {ant.position.first + Ant::dr[moveDir],
                  ant.position.second + Ant::dc[moveDir]};

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


    // If ant fails, try all other directions
    if (ant.position == ant.move(world->terrainMap, step, world->foodMap)) {
        bool canMove = ant.anyLegalMove(world->terrainMap, world->foodMap);

        // If ant can't move in any direction, it's trapped and should be
        // destroyed.
        if (!canMove) {
#ifdef DEBUG_STATE_TRANSITION
            logStateTransition(ant.antID, "RandomExploration", "Combust", "no more legal move");
#endif
            ant.combust();
            return;
        }
    }

    ant.dropPheromone(world->pheromoneMap, PheromoneType::Position, 2);

    /* Regardless of what happens, always transition back to
     * DeterminedExploration for reward update, which includes checking for food
     * and pheromone and transitioning to the appropriate state */
#ifdef DEBUG_STATE_TRANSITION
    logStateTransition(ant.antID, "RandomExploration", "DeterminedExploration", "finished random move");
#endif
    ant.switchTo(DeterminedExploration{}, world);

}
