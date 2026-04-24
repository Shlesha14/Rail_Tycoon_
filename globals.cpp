/*
 * globals.cpp
 * -----------
 * Defines every global variable declared extern in rail_empire.h.
 * Only this file allocates storage - all other files just reference it.
 */

#include "rail_empire.h"

vector<Node>  nodes;
vector<Route> routes;
vector<Train> trains;

Finance pFin, aFin;
int     playerBudget = 2500;
int     aiBudget     = 2500;
int     selected     = -1;
int     hovered      = -1;

GameState state     = S_BUILD;
AlgoMode  algoMode  = ALGO_DIJKSTRA;
bool usePrim      = false;
bool showSPT      = true;
bool showViz      = true;
bool showMST      = true;
bool showPlayer   = true;
bool showAlgoLog  = true;

int           pathSrc = -1;
vector<int>   pathPrev;
vector<float> pathDist;
vector<bool>  bfsVisited;
string        algoLog;

sf::Font font;

map<string,string> complexity = {
    {"Kruskal",     "O(E log E)"},
    {"Prim",        "O((V+E) log V)"},
    {"Dijkstra",    "O((V+E) log V)"},
    {"Bellman-Ford","O(VxE)"},
    {"BFS",         "O(V+E)"},
    {"MergeSort",   "O(n log n)"},
    {"Knapsack",    "O(nxW)"},
    {"DSU find",    "O(a(n)) ~O(1)"}
};
