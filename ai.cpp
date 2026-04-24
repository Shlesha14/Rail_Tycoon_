/*
 * ai.cpp
 * ------
 * The AI opponent's network-building engine.
 *
 * runAI() executes a 4-step pipeline when the player presses SPACE:
 *
 *   Step 1 - Kruskal or Prim MST
 *             Minimum-cost backbone connecting all 7 cities.
 *             Produces green routes on the map.
 *
 *   Step 2 - Dijkstra SPT from richest city
 *             Direct connections from the highest-demand city.
 *             Produces purple routes on the map.
 *
 *   Step 3 - Greedy scoring + MergeSort
 *             Score every industry->city pair as demand*100/distance.
 *             Sort best-first. Keep one best city per industry.
 *
 *   Step 4 - AI picks 2nd or 3rd best route (intentionally beatable)
 *             Makes the AI fair for the player to compete against.
 *
 * Note: buildSPTFromCity() writes to global pathPrev/pathDist.
 * We do NOT snapshot here since the original code didn't either -
 * preserving identical behaviour.
 */

#include "rail_empire.h"
#include <set>
#include <algorithm>
using namespace std;

// Forward declarations from other .cpp files
vector<pair<int,int>> kruskalMST();
vector<pair<int,int>> primMST();
void buildSPTFromCity(int src);
void mergeSort(vector<RouteOption>& v, int left, int right);
bool addRoute(int u, int v, bool isAI, RouteType rt);

void runAI(){
    algoLog = "=== AI TURN ===\n";

    // ---- Step 1: MST backbone (Kruskal or Prim) ----
    auto mst = usePrim ? primMST() : kruskalMST();
    int mstBuilt = 0;
    for(auto [u, v] : mst)
        if(addRoute(u, v, true, RT_MST)) mstBuilt++;
    algoLog += "MST routes built: " + to_string(mstBuilt) + "\n";

    // ---- Step 2: Dijkstra SPT from highest-demand city ----
    int richestCity = -1, maxDemand = 0;
    for(auto& nd : nodes){
        if(nd.type != CITY) continue;
        int total = nd.demand[FOOD] + nd.demand[GOODS] + nd.demand[PASSENGERS];
        if(total > maxDemand){ maxDemand = total; richestCity = nd.id; }
    }

    int sptBuilt = 0;
    if(richestCity >= 0){
        buildSPTFromCity(richestCity);
        for(int i = 0; i < N(); i++)
            if(pathPrev[i] != -1 && pathDist[i] < 1e8f
               && nodes[i].type == CITY && nodes[pathPrev[i]].type == CITY)
                if(addRoute(pathPrev[i], i, true, RT_SPT)) sptBuilt++;
        algoLog += "SPT city " + to_string(richestCity)
                 + " (max demand): " + to_string(sptBuilt) + " routes\n";
    }

    // ---- Step 3: Greedy scoring + MergeSort ----
    algoLog += "\n-- GREEDY SCORING --\n";

    vector<RouteOption> opts;
    for(auto& src : nodes){
        if(src.type == CITY) continue;
        for(auto& dst : nodes){
            if(dst.type != CITY) continue;
            float d = vdist(src.pos, dst.pos);
            if(d < 10.f) continue;
            int demand = dst.demand[src.produces];
            if(demand <= 0) continue;
            int score = (int)(demand * 100.f / d);
            int cost  = (int)(d * ROUTE_COST_PER_PX);
            opts.push_back({src.id, dst.id, cost, score});
        }
    }

    if(!opts.empty()) mergeSort(opts, 0, (int)opts.size()-1);
    algoLog += "MergeSort " + to_string(opts.size()) + " opts\n";

    // Keep one best city per industry source
    vector<RouteOption> topOpts;
    set<int> usedSrc;
    for(auto& o : opts){
        if(usedSrc.count(o.u)) continue;
        usedSrc.insert(o.u);
        topOpts.push_back(o);
        algoLog += " "+to_string(o.u)+"->"+to_string(o.v)+" sc="+to_string(o.score)+"\n";
        if((int)topOpts.size() >= 3) break;
    }

    // ---- Step 4: AI picks 2nd or 3rd best (intentionally beatable) ----
    if(!topOpts.empty()){
        int idx = 0;
        if((int)topOpts.size() >= 3)
            idx = 1 + (rand() % 2);   // randomly pick 2nd or 3rd best

        if(topOpts[idx].cost <= aiBudget){
            addRoute(topOpts[idx].u, topOpts[idx].v, true, RT_AI_PROFIT);
            algoLog += "AI chose route: "
                     + to_string(topOpts[idx].u) + "->"
                     + to_string(topOpts[idx].v) + "\n";
        } else {
            algoLog += "AI could not afford chosen route\n";
        }
    }
}
