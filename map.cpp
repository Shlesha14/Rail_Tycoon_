/*
 * map.cpp
 * -------
 * Generates the game world - 16 nodes across 4 supply-chain zones.
 *
 * Zone layout (left to right on screen):
 *   A (x~120) : 3 Farms       - produce Food
 *   B (x~280) : 2 Coal Mines  - produce Coal
 *   C (x~470) : 2 Factories   - produce Goods
 *   D (x~820) : 7 Cities      - demand Food, Goods, Passengers
 *   Extra     : 2 Stations    - passenger hubs
 *
 * Every position uses a small random jitter so the map looks different
 * each game but nodes always stay within their correct zone.
 * City demand values are randomised - optimal routes change every game.
 */

#include "rail_empire.h"
#include <cstdlib>
#include <ctime>

void generateMap(){
    srand((unsigned)time(nullptr));
    nodes.clear();
    routes.clear();
    trains.clear();

    // jit(base, range) - random value within +/- range/2 of base
    auto jit = [](int base, int range){
        return base + (rand() % range) - range / 2;
    };

    // add() - creates one node and appends it to the global nodes vector
    auto add = [&](NodeType type, float x, float y,
                   Cargo prod = FOOD, int pop = 0,
                   int dFood = 0, int dGoods = 0, int dPass = 0){
        Node n;
        n.id             = (int)nodes.size();  // ID = index in the vector
        n.pos            = {x, y};
        n.type           = type;
        n.produces       = prod;
        n.stock          = 80 + rand() % 40;   // random starting stock 80-119
        n.maxStock       = 200;
        n.population     = pop;
        n.demand[FOOD]       = dFood;
        n.demand[GOODS]      = dGoods;
        n.demand[PASSENGERS] = dPass;
        nodes.push_back(n);
    };

    // Zone A - Farms (IDs 0, 1, 2)
    add(FARM,      jit(120,40), jit(160,40), FOOD);
    add(FARM,      jit(120,40), jit(420,40), FOOD);
    add(FARM,      jit(120,40), jit(680,40), FOOD);

    // Zone B - Coal Mines (IDs 3, 4)
    add(COAL_MINE, jit(280,40), jit(230,40), COAL);
    add(COAL_MINE, jit(280,40), jit(600,40), COAL);

    // Zone C - Factories (IDs 5, 6)
    add(FACTORY,   jit(470,40), jit(340,40), GOODS);
    add(FACTORY,   jit(470,40), jit(560,40), GOODS);

    // Zone D - Cities (IDs 7-13)
    // Cities alternate between two x-columns to prevent label overlap
    int cityYs[] = {70, 150, 240, 330, 420, 510, 600};
    for(int i = 0; i < 7; i++){
        int pop   = 4000 + rand() % 5000;
        int dFood = 60   + rand() % 60;   // Food demand   60-119
        int dGoods= 50   + rand() % 60;   // Goods demand  50-109
        int dPass = 70   + rand() % 70;   // Passenger demand 70-139
        add(CITY, jit(820,50) + (i%2)*50, cityYs[i],
            FOOD, pop, dFood, dGoods, dPass);
    }

    // Stations (IDs 14, 15) - passenger hubs
    add(STATION, jit(700,40), 250, PASSENGERS);
    add(STATION, jit(700,40), 560, PASSENGERS);
}
