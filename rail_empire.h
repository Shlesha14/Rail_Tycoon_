#pragma once
/*
 * rail_empire.h
 * -------------
 * Single shared header included by every .cpp file.
 * Contains: constants, enums, structs, extern globals, inline helpers.
 * Actual global storage lives in globals.cpp.
 */

#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <map>
using namespace std;

// =========================================================================
//  CONSTANTS
// =========================================================================
const int   WIN_W               = 1300;
const int   WIN_H               = 740;
const int   PANEL               = 320;
const int   MAP_W               = WIN_W - PANEL;
const float NODE_R              = 11.f;
const int   SIM_STEPS           = 600;

// Economy tuning - named so magic numbers are self-documenting
const float ROUTE_COST_PER_PX   = 0.35f;
const float CARGO_PROFIT_PER_PX = 0.055f;
const float PASSEN_PROFIT_PER_PX= 0.040f;
const float MAINT_PER_PX        = 0.015f;

// =========================================================================
//  ENUMS
// =========================================================================
enum Cargo     { FOOD=0, COAL, GOODS, PASSENGERS };
enum NodeType  { CITY=0, FARM, COAL_MINE, FACTORY, STATION };
enum RouteType { RT_MST=0, RT_SPT, RT_PLAYER, RT_AI_PROFIT };
enum GameState { S_BUILD=0, S_PLAYER_RESULT, S_AI_RESULT, S_FINAL };
enum AlgoMode  { ALGO_DIJKSTRA=0, ALGO_BELLMAN, ALGO_BFS };

// =========================================================================
//  STRUCTS
// =========================================================================
struct Node {
    int          id;
    sf::Vector2f pos;
    NodeType     type;
    Cargo        produces   = FOOD;
    int          stock      = 100;
    int          maxStock   = 200;
    int          population = 0;
    int          demand[4]  = {0,0,0,0};  // indexed by Cargo enum
};

struct Route {
    int       u, v;           // endpoint node IDs
    float     len;            // pixel distance
    bool      isAI;
    Cargo     cargo;
    RouteType rtype;
    int       totalProfit = 0;
    int       trips       = 0;
};

struct Train {
    int   routeIdx;
    float t       = 0.f;   // 0 = at node u,  1 = at node v
    bool  forward = true;
};

struct Finance {
    int revenue=0, buildCost=0, maint=0;
    int net() const { return revenue - buildCost - maint; }
};

// Used by MergeSort + Knapsack (ai.cpp)
struct RouteOption {
    int u, v, cost, score;
};

// =========================================================================
//  EXTERN GLOBALS  (defined in globals.cpp)
// =========================================================================
extern vector<Node>  nodes;
extern vector<Route> routes;
extern vector<Train> trains;

extern Finance pFin, aFin;
extern int     playerBudget, aiBudget;
extern int     selected, hovered;

extern GameState state;
extern AlgoMode  algoMode;
extern bool usePrim, showSPT, showViz, showMST, showPlayer, showAlgoLog;

extern int           pathSrc;
extern vector<int>   pathPrev;
extern vector<float> pathDist;
extern vector<bool>  bfsVisited;
extern string        algoLog;

extern sf::Font              font;
extern map<string,string>    complexity;

// =========================================================================
//  INLINE HELPERS  (used everywhere, tiny enough to live in the header)
// =========================================================================
inline float        vdist(sf::Vector2f a, sf::Vector2f b){ return hypotf(a.x-b.x,a.y-b.y); }
inline sf::Vector2f npos(int id)  { return nodes[id].pos; }
inline int          N()           { return (int)nodes.size(); }

inline sf::Color cargoCol(Cargo c){
    switch(c){
        case FOOD:       return {80,220,80};
        case COAL:       return {160,160,160};
        case GOODS:      return {255,180,50};
        case PASSENGERS: return {100,200,255};
    }
    return sf::Color::White;
}

inline sf::Color nodeCol(const Node& n){
    switch(n.type){
        case CITY:      return {70,150,255};
        case FARM:      return {50,200,50};
        case COAL_MINE: return {150,150,150};
        case FACTORY:   return {255,130,30};
        case STATION:   return {170,120,255};
    }
    return sf::Color::White;
}
