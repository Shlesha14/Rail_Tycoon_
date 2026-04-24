/*
 * algorithms.cpp
 * --------------
 * All DAA algorithm implementations mapped to TCS409 syllabus units.
 *
 *  Unit II  : mergeSort()            O(n log n)
 *  Unit III : DSU                    O(alpha(n)) per operation
 *             runBFS()               O(V+E)
 *             runDijkstra()          O((V+E) log V)
 *             runBellmanFord()       O(V x E)
 *             kruskalMST()           O(E log E)
 *             primMST()              O((V+E) log V)
 *             buildSPTFromCity()     O((V+E) log V)  AI-internal
 *  Unit IV  : knapsackSelectRoutes() O(n x W)
 *
 * Every function writes step-by-step output into the global algoLog
 * string which is displayed in the HUD panel in real time.
 */

#include "rail_empire.h"
#include <algorithm>
#include <queue>
#include <numeric>
using namespace std;

// =========================================================================
//  UNIT II - MERGE SORT
// =========================================================================
/*
 * Sorts RouteOption candidates by score descending.
 * Called in ai.cpp before Knapsack to rank all candidates best-first.
 */
void mergeSort(vector<RouteOption>& v, int left, int right){
    if(left >= right) return;

    int mid = (left + right) / 2;
    mergeSort(v, left, mid);
    mergeSort(v, mid+1, right);

    // Merge: pick the higher-scored element from each half
    vector<RouteOption> tmp;
    int i = left, j = mid+1;
    while(i <= mid && j <= right)
        tmp.push_back(v[i].score >= v[j].score ? v[i++] : v[j++]);
    while(i <= mid)  tmp.push_back(v[i++]);
    while(j <= right) tmp.push_back(v[j++]);

    for(int k = left; k <= right; k++)
        v[k] = tmp[k - left];
}

// =========================================================================
//  UNIT III - DSU (Disjoint Set Union / Union-Find)
// =========================================================================
/*
 * Used inside kruskalMST() to check whether adding an edge forms a cycle.
 * Path compression + union by rank give O(alpha(n)) ~ O(1) per operation.
 */
struct DSU {
    vector<int> parent, rank;

    DSU(int n) : parent(n), rank(n, 0){
        iota(parent.begin(), parent.end(), 0);
    }

    // find() with path compression - returns root of x's set
    int find(int x){
        return parent[x] == x ? x : parent[x] = find(parent[x]);
    }

    // unite() - merges the sets of a and b
    // Returns false if already in same set (adding edge would create a cycle)
    bool unite(int a, int b){
        a = find(a); b = find(b);
        if(a == b) return false;
        if(rank[a] < rank[b]) swap(a, b);
        parent[b] = a;
        if(rank[a] == rank[b]) rank[a]++;
        return true;
    }
};

// =========================================================================
//  UNIT III - BFS (Breadth-First Search)
// =========================================================================
/*
 * Explores all nodes reachable from src, level by level (nearest first).
 * Triggered by: F key then right-click, or C key (instant from node 0).
 * Visual result: cyan rings on every reachable node.
 */
void runBFS(int src){
    bfsVisited.assign(N(), false);

    vector<vector<int>> adj(N());
    for(auto& r : routes){
        adj[r.u].push_back(r.v);
        adj[r.v].push_back(r.u);
    }

    queue<int> q;
    q.push(src);
    bfsVisited[src] = true;

    algoLog  = "-- BFS from node " + to_string(src) + " --\n";
    algoLog += complexity["BFS"] + "\n\nVisit order:\n";

    int order = 0;
    while(!q.empty()){
        int u = q.front(); q.pop();
        algoLog += to_string(u);
        if(++order % 8 == 0) algoLog += "\n";
        else                  algoLog += " ";
        for(int v : adj[u])
            if(!bfsVisited[v]){ bfsVisited[v] = true; q.push(v); }
    }

    pathSrc = src;
}

// =========================================================================
//  UNIT III - DIJKSTRA (Single Source Shortest Path)
// =========================================================================
/*
 * Uses a min-heap. Always processes the cheapest unvisited node first.
 * Non-negative edge weights only.
 * Triggered by: D key then right-click.
 * Visual result: yellow lines following pathPrev[] parent pointers.
 */
void runDijkstra(int src){
    pathDist.assign(N(), 1e9f);
    pathPrev.assign(N(), -1);
    pathDist[src] = 0.f;

    vector<vector<pair<int,float>>> adj(N());
    for(auto& r : routes){
        adj[r.u].push_back({r.v, r.len});
        adj[r.v].push_back({r.u, r.len});
    }

    priority_queue<pair<float,int>,
                   vector<pair<float,int>>,
                   greater<>> pq;
    pq.push({0.f, src});

    while(!pq.empty()){
        auto [d, u] = pq.top(); pq.pop();
        if(d > pathDist[u]) continue;   // stale entry, skip
        for(auto [v, w] : adj[u])
            if(pathDist[u]+w < pathDist[v]){
                pathDist[v] = pathDist[u]+w;
                pathPrev[v] = u;
                pq.push({pathDist[v], v});
            }
    }

    pathSrc = src;
    algoLog  = "-- DIJKSTRA SSSP --\n" + complexity["Dijkstra"]
             + "\nSource: Node " + to_string(src) + "\n\nNode -> Dist  Parent\n";
    for(int i = 0; i < N(); i++)
        if(pathDist[i] < 1e8f)
            algoLog += " " + to_string(i) + "  ->  "
                     + to_string((int)pathDist[i])
                     + "  (par:" + to_string(pathPrev[i]) + ")\n";
}

// =========================================================================
//  UNIT III - BELLMAN-FORD (Single Source Shortest Path)
// =========================================================================
/*
 * Relaxes every edge V-1 times. Handles negative edge weights.
 * Early stop: if a full pass produces no update, result is optimal.
 * Triggered by: B key then right-click.
 */
void runBellmanFord(int src){
    pathDist.assign(N(), 1e9f);
    pathPrev.assign(N(), -1);
    pathDist[src] = 0.f;

    vector<tuple<int,int,float>> edges;
    for(auto& r : routes){
        edges.push_back({r.u, r.v, r.len});
        edges.push_back({r.v, r.u, r.len});
    }

    algoLog  = "-- BELLMAN-FORD SSSP --\n" + complexity["Bellman-Ford"]
             + "\nSource: Node " + to_string(src)
             + "\nRelaxing " + to_string(N()-1) + " iterations...\n\n";

    for(int i = 0; i < N()-1; i++){
        bool updated = false;
        for(auto [u, v, w] : edges)
            if(pathDist[u] < 1e8f && pathDist[u]+w < pathDist[v]){
                pathDist[v] = pathDist[u]+w;
                pathPrev[v] = u;
                updated = true;
            }
        if(!updated){
            algoLog += "Early stop at iteration " + to_string(i) + "\n";
            break;
        }
    }

    algoLog += "\nResult (no -ve cycle):\n";
    for(int i = 0; i < N(); i++)
        if(pathDist[i] < 1e8f)
            algoLog += " " + to_string(i) + " dist=" + to_string((int)pathDist[i]) + "\n";

    pathSrc = src;
}

// =========================================================================
//  UNIT III - KRUSKAL MST
// =========================================================================
/*
 * Builds a Minimum Spanning Tree over all City nodes.
 * Steps:
 *   1. Sort all city-city edges by distance     O(E log E)
 *   2. Add each edge that doesn't form a cycle  (DSU check)
 *   3. Stop when N-1 edges added                (tree complete)
 */
vector<pair<int,int>> kruskalMST(){
    vector<int> cityIds;
    for(int i = 0; i < N(); i++)
        if(nodes[i].type == CITY) cityIds.push_back(i);

    vector<tuple<float,int,int>> edges;
    for(int a = 0; a < (int)cityIds.size(); a++)
        for(int b = a+1; b < (int)cityIds.size(); b++)
            edges.push_back({vdist(npos(cityIds[a]),npos(cityIds[b])),
                             cityIds[a], cityIds[b]});
    sort(edges.begin(), edges.end());

    DSU dsu(N());
    vector<pair<int,int>> mst;
    algoLog  = "KRUSKAL MST (cities)\n" + complexity["Kruskal"]
             + "\n" + to_string(edges.size()) + " city edges\n\n";

    for(auto& [w, u, v] : edges){
        if(dsu.unite(u, v)){
            mst.push_back({u, v});
            algoLog += "C"+to_string(u)+"-C"+to_string(v)+" w="+to_string((int)w)+"\n";
            if((int)mst.size() == (int)cityIds.size()-1) break;
        }
    }

    algoLog += "\nMST: " + to_string(mst.size()) + " edges";
    return mst;
}

// =========================================================================
//  UNIT III - PRIM MST
// =========================================================================
/*
 * Alternative MST: grows the tree one node at a time from a starting city.
 * Always picks the cheapest edge connecting the current tree to an outside node.
 * Toggle between Kruskal and Prim with the P key.
 */
vector<pair<int,int>> primMST(){
    vector<int> cityIds;
    for(int i = 0; i < N(); i++)
        if(nodes[i].type == CITY) cityIds.push_back(i);
    if(cityIds.empty()) return {};

    vector<float> minEdge(N(), 1e9f);
    vector<int>   parent(N(), -1);
    vector<bool>  inMST(N(), false);
    minEdge[cityIds[0]] = 0.f;

    priority_queue<pair<float,int>,
                   vector<pair<float,int>>,
                   greater<>> pq;
    pq.push({0.f, cityIds[0]});

    algoLog = "PRIM MST (cities)\n" + complexity["Prim"] + "\n\n";
    vector<pair<int,int>> mst;

    while(!pq.empty()){
        auto [d, u] = pq.top(); pq.pop();
        if(inMST[u] || nodes[u].type != CITY) continue;
        inMST[u] = true;
        if(parent[u] != -1){
            mst.push_back({parent[u], u});
            algoLog += "C"+to_string(parent[u])+"-C"+to_string(u)+" w="+to_string((int)d)+"\n";
        }
        for(int v : cityIds) if(!inMST[v]){
            float w = vdist(npos(u), npos(v));
            if(w < minEdge[v]){ minEdge[v]=w; parent[v]=u; pq.push({w,v}); }
        }
    }

    algoLog += "\nMST: " + to_string(mst.size()) + " edges";
    return mst;
}

// =========================================================================
//  UNIT III - SPT FROM CITY (AI internal)
// =========================================================================
/*
 * Dijkstra Shortest Path Tree over the complete city-city distance graph.
 * Called only by runAI() Step 2 - not exposed as a player-facing feature.
 */
void buildSPTFromCity(int src){
    pathDist.assign(N(), 1e9f);
    pathPrev.assign(N(), -1);
    pathDist[src] = 0.f;

    vector<vector<pair<int,float>>> adj(N());
    for(int i = 0; i < N(); i++){
        if(nodes[i].type != CITY) continue;
        for(int j = 0; j < N(); j++){
            if(i==j || nodes[j].type != CITY) continue;
            adj[i].push_back({j, vdist(npos(i), npos(j))});
        }
    }

    priority_queue<pair<float,int>,
                   vector<pair<float,int>>,
                   greater<>> pq;
    pq.push({0.f, src});

    while(!pq.empty()){
        auto [d, u] = pq.top(); pq.pop();
        if(d > pathDist[u]) continue;
        for(auto [v, w] : adj[u])
            if(pathDist[u]+w < pathDist[v]){
                pathDist[v] = pathDist[u]+w;
                pathPrev[v] = u;
                pq.push({pathDist[v], v});
            }
    }
    pathSrc = src;
}

// =========================================================================
//  UNIT IV - 0/1 KNAPSACK DP
// =========================================================================
/*
 * Selects the optimal subset of route candidates within a budget.
 *
 * dp[i][j] = max score using first i candidates with capacity j
 *
 * Recurrence:
 *   dp[i][j] = max( dp[i-1][j],              // skip candidate i
 *                   dp[i-1][j-w] + score_i )  // take candidate i
 *
 * Backtracking recovers which candidates were actually chosen.
 */
vector<int> knapsackSelectRoutes(vector<RouteOption>& opts, int budget){
    int n = (int)opts.size();
    int W = budget / 10;   // scale down to keep table manageable

    vector<vector<int>> dp(n+1, vector<int>(W+1, 0));
    for(int i = 1; i <= n; i++){
        int w     = opts[i-1].cost  / 10;
        int score = opts[i-1].score;
        for(int j = 0; j <= W; j++){
            dp[i][j] = dp[i-1][j];
            if(j >= w && dp[i-1][j-w]+score > dp[i][j])
                dp[i][j] = dp[i-1][j-w]+score;
        }
    }

    // Backtrack
    vector<int> chosen;
    int j = W;
    for(int i = n; i >= 1; i--){
        int w = opts[i-1].cost / 10;
        if(j >= w && dp[i][j] == dp[i-1][j-w]+opts[i-1].score){
            chosen.push_back(i-1);
            j -= w;
        }
    }

    algoLog += "\n-- 0/1 KNAPSACK (DP) --\n" + complexity["Knapsack"]
             + "\nBudget: $" + to_string(budget)
             + "  Options: " + to_string(n) + "\n"
             + "Selected " + to_string(chosen.size()) + " routes\n"
             + "Max score: " + to_string(dp[n][W]) + "\n";
    return chosen;
}
