#include "shortest_path.hpp"

#include <algorithm>
#include <functional>
#include <limits>
#include <queue>
#include <utility>

namespace {
const double INF = std::numeric_limits<double>::infinity();
}

ShortestPaths dijkstra(const VisibilityGraph& g, int s) {
    const int n = g.size();
    ShortestPaths sp;
    sp.dist.assign(n, INF);
    sp.parent.assign(n, -1);

    using Entry = std::pair<double, int>;
    std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> q;
    sp.dist[s] = 0;
    q.push({0.0, s});

    while (!q.empty()) {
        auto [d, u] = q.top();
        q.pop();
        if (d > sp.dist[u]) continue;  // entrada obsoleta
        for (const Edge& e : g.adj[u]) {
            double nd = d + e.w;
            if (nd < sp.dist[e.to]) {
                sp.dist[e.to] = nd;
                sp.parent[e.to] = u;
                q.push({nd, e.to});
            }
        }
    }
    return sp;
}

std::vector<int> reconstructPath(const ShortestPaths& sp, int s, int t) {
    std::vector<int> path;
    if (sp.dist[t] == INF) return path;
    for (int v = t; v != -1; v = sp.parent[v]) {
        path.push_back(v);
        if (v == s) break;
    }
    std::reverse(path.begin(), path.end());
    return path;
}

std::vector<std::string> pathLabels(const VisibilityGraph& g, const std::vector<int>& path) {
    std::vector<std::string> out;
    for (int v : path) out.push_back(nodeLabel(g.nodes[v]));
    return out;
}

std::vector<std::vector<double>> floydWarshall(const VisibilityGraph& g) {
    const int n = g.size();
    std::vector<std::vector<double>> d(n, std::vector<double>(n, INF));
    for (int i = 0; i < n; i++) {
        d[i][i] = 0;
        for (const Edge& e : g.adj[i]) d[i][e.to] = std::min(d[i][e.to], e.w);
    }
    for (int k = 0; k < n; k++)
        for (int i = 0; i < n; i++) {
            if (d[i][k] == INF) continue;
            for (int j = 0; j < n; j++)
                if (d[i][k] + d[k][j] < d[i][j]) d[i][j] = d[i][k] + d[k][j];
        }
    return d;
}
