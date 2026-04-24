/*
 * routes.cpp
 * ----------
 * Route management and the 600-tick simulation engine.
 *
 *   routeExists() - prevents duplicate routes
 *   addRoute()    - validates, builds a route, spawns a train
 *   simulate()    - runs the economy loop for one player (player or AI)
 */

#include "rail_empire.h"
#include <algorithm>
using namespace std;

// Returns true if a route between u and v already exists (either direction)
bool routeExists(int u, int v){
    for(auto& r : routes)
        if((r.u==u && r.v==v) || (r.u==v && r.v==u)) return true;
    return false;
}

/*
 * addRoute() - attempts to build a route between nodes u and v.
 * Returns false (no side effects) when:
 *   - u == v (self-loop)
 *   - route already exists
 *   - owner cannot afford the build cost
 *
 * On success: deducts cost, pushes Route + Train into global vectors.
 * Cargo type is decided at creation time from the source node's .produces field.
 */
bool addRoute(int u, int v, bool isAI, RouteType rt){
    if(u == v || routeExists(u, v)) return false;

    float dist = vdist(npos(u), npos(v));
    int   cost = max(10, (int)(dist * ROUTE_COST_PER_PX));

    Finance& fin  = isAI ? aFin : pFin;
    int&     money = isAI ? aiBudget : playerBudget;
    if(money < cost) return false;

    money         -= cost;
    fin.buildCost += cost;

    Cargo cargo = (nodes[u].type == CITY) ? PASSENGERS : nodes[u].produces;
    routes.push_back({u, v, dist, isAI, cargo, rt, 0, 0});

    float startT = (float)(rand() % 100) / 100.f;
    trains.push_back({(int)routes.size()-1, startT, true});
    return true;
}

/*
 * simulate() - runs SIM_STEPS (600) ticks for one side (player or AI).
 *
 * Each tick:
 *   1. Every owned route ships cargo and earns profit minus maintenance
 *   2. Cities regenerate demand; industries regenerate stock
 *
 * Three route cases:
 *   City <-> City  : passenger route
 *   Station -> City: passenger hub route
 *   Industry -> City: cargo route (Food / Coal / Goods)
 */
void simulate(bool isAI){
    Finance& fin  = isAI ? aFin : pFin;
    int&     money = isAI ? aiBudget : playerBudget;

    for(int step = 0; step < SIM_STEPS; step++){

        for(auto& r : routes){
            if(r.isAI != isAI) continue;

            Node& src = nodes[r.u];
            Node& dst = nodes[r.v];

            if(src.type == CITY && dst.type == CITY){
                int d = min(src.demand[PASSENGERS], 15);
                if(d <= 0) continue;
                src.demand[PASSENGERS] -= d;
                int profit = (int)(d * r.len * PASSEN_PROFIT_PER_PX);
                int maint  = (int)(r.len * MAINT_PER_PX);
                money += profit - maint;
                fin.revenue += profit; fin.maint += maint;
                r.totalProfit += profit; r.trips++;
            }
            else if(src.type == STATION && dst.type == CITY){
                int d = min(src.stock, 15);
                if(d <= 0) continue;
                src.stock -= d;
                int profit = (int)(d * r.len * PASSEN_PROFIT_PER_PX);
                int maint  = (int)(r.len * MAINT_PER_PX);
                money += profit - maint;
                fin.revenue += profit; fin.maint += maint;
                r.totalProfit += profit; r.trips++;
            }
            else if(src.type != CITY && src.type != STATION && dst.type == CITY){
                int demand = dst.demand[r.cargo];
                int d = min({src.stock, demand, 20});
                if(d <= 0) continue;
                src.stock           -= d;
                dst.demand[r.cargo] -= d;
                int profit = (int)(d * r.len * CARGO_PROFIT_PER_PX);
                int maint  = (int)(r.len * MAINT_PER_PX);
                money += profit - maint;
                fin.revenue += profit; fin.maint += maint;
                r.totalProfit += profit; r.trips++;
            }
        }

        // Regenerate demand and stock every tick
        for(auto& nd : nodes){
            if(nd.type == CITY){
                nd.demand[FOOD]       += 2;
                nd.demand[GOODS]      += 2;
                nd.demand[PASSENGERS] += 3;
            } else {
                nd.stock = min(nd.maxStock, nd.stock + 2);
            }
        }
    }
}
