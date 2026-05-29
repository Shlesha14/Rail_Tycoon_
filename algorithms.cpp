#include "globals.h"
#include <algorithm>
#include <queue>
using namespace std;

// MERGE SORT
void mergeSort(vector<RouteOption>& v, int left, int right) {
    if (left >= right) 
    return;
    int mid = (left + right) / 2;
    mergeSort(v, left, mid);
    mergeSort(v, mid + 1, right);
    vector<RouteOption> temp;
    int i = left, j = mid + 1;
    while (i <= mid && j <= right) 
    {
        if (v[i].score >= v[j].score)
            temp.push_back(v[i++]);
        else
            temp.push_back(v[j++]);
    }
    while (i <= mid)  
        temp.push_back(v[i++]);
    while (j <= right) 
        temp.push_back(v[j++]);
    for (int k = left; k <= right; k++)
        v[k] = temp[k - left];
}

//DSU (Disjoint Set Union)
//used by kruskalMST to check if by adding an edge it creates a cycle
struct DSU {
    vector<int> parent, rank;
    DSU(int n){
        parent.resize(n);
        rank.resize(n,0);
        for (int i = 0; i < n; i++)
            parent[i] = i;
    }
    int find(int x)
    {
        if (parent[x] != x)
            parent[x] = find(parent[x]); // path compression
        return parent[x];
    }
    bool unite(int a, int b) 
    {
        int ra = find(a), rb = find(b);
        if (ra == rb) 
        return false;
        if (rank[ra] < rank[rb]) 
        swap(ra, rb);
        parent[rb] = ra;
        if (rank[ra] == rank[rb]) 
        //increment height if both have same height
        rank[ra]++;
        return true;
    }
};
//DIJKSTRA
//finds shortest path from src to all other reachable nodes
void runDijkstra(int src) {
    pathDist.assign(N(), 1e9f);
    pathPrev.assign(N(), -1);
    pathDist[src] = 0.0f;
    vector<vector<pair<int, float>>> adj(N());
    for (int i = 0; i < (int)routes.size(); i++) {
        adj[routes[i].u].push_back({routes[i].v, routes[i].len});
        adj[routes[i].v].push_back({routes[i].u, routes[i].len});
    } // src to dest and dest to src

    priority_queue<pair<float,int>, vector<pair<float,int>>, greater<>> pq;
    pq.push({0.0f, src});
    while (!pq.empty()) {
        pair<float,int> temp = pq.top();
        float dist = temp.first;
        int u = temp.second;
        pq.pop();
        if (dist > pathDist[u]) continue; // skip if we already found a shorter path to u
        for (auto [v, weight] : adj[u]) {
            float newDist = pathDist[u] + weight;
            if (newDist < pathDist[v]) {
                pathDist[v] = newDist;
                pathPrev[v] = u;
                pq.push({newDist, v});
            }
        }
    }
    pathSrc = src;
}

// KRUSKAL MST
// connects all cities with minimum distance
vector<pair<int,int>> kruskalMST() 
{
    vector<int> cities;
    for (int i = 0; i < N(); i++)
        if (nodes[i].type == CITY)
            cities.push_back(i);
    // generate every possible city-city edge
    vector<tuple<float, int, int>> edges;
    for (int a = 0; a < (int)cities.size(); a++) 
    {
        for (int b = a + 1; b < (int)cities.size(); b++) 
        {
            float d = vdist(npos(cities[a]), npos(cities[b]));
            edges.push_back({d, cities[a], cities[b]});
        }
    }
    // sort edges shortest first
    sort(edges.begin(), edges.end());
    DSU dsu(N());
    vector<pair<int,int>> mst;
    for (auto& [w, u, v] : edges) 
    {
        if (dsu.unite(u, v)) 
        {
            mst.push_back({u, v});
            if ((int)mst.size() == (int)cities.size() - 1)
                break;
        }
    }
    return mst;
}

// SPT
// finds shortest path tree from the richest city
void buildSPTFromCity(int src) 
{
    pathDist.assign(N(), 1e9f);
    pathPrev.assign(N(), -1);
    pathDist[src] = 0.0f;

    vector<vector<pair<int, float>>> adj(N());
    for (int i = 0; i < N(); i++) 
    {
        if (nodes[i].type != CITY) continue;
        for (int j = 0; j < N(); j++) 
        {
            if (i == j || nodes[j].type != CITY) continue;
            adj[i].push_back({j, vdist(npos(i), npos(j))});
        }
    }

    priority_queue<pair<float,int>, vector<pair<float,int>>, greater<>> pq;
    pq.push({0.0f, src});
    while (!pq.empty()) 
    {
        auto [d, u] = pq.top();
        pq.pop();
        if (d > pathDist[u]) continue;
        for (auto [v, w] : adj[u]) 
        {
            if (pathDist[u] + w < pathDist[v]) 
            {
                pathDist[v] = pathDist[u] + w;
                pathPrev[v] = u;
                pq.push({pathDist[v], v});
            }
        }
    }
    pathSrc = src;
}

//0/1 KNAPSACK
vector<int> knapsackSelectRoutes(vector<RouteOption>& opts, int budget) {
    int n = (int)opts.size();
    int W = budget / 10;

    vector<vector<int>> dp(n + 1, vector<int>(W + 1, 0));
    for (int i = 1; i <= n; i++) {
        int w = opts[i-1].cost / 10;
        int s = opts[i-1].score;
        for (int j = 0; j <= W; j++) {
            dp[i][j] = dp[i-1][j]; // skip this route
            if (j >= w && dp[i-1][j-w] + s > dp[i][j])
                dp[i][j] = dp[i-1][j-w] + s; // take this route
        }
    }
    // backtrack to find which routes were selected
    vector<int> chosen;
    int j = W;
    for (int i = n; i >= 1; i--) {
        int w = opts[i-1].cost / 10;
        if (j >= w && dp[i][j] == dp[i-1][j-w] + opts[i-1].score) {
            chosen.push_back(i - 1);
            j -= w;
        }
    }
    return chosen;
}
