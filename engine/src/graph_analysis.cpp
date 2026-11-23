#include "graph_analysis.hpp"

#include <algorithm>
#include <iomanip>
#include <limits>
#include <numeric>
#include <sstream>

#include "shortest_path.hpp"

namespace {

const double INF = std::numeric_limits<double>::infinity();
const double EPS = 1e-9;

struct REdge {
    int u, v;
    double w;
};

// Union-busqueda con compresion de caminos y union por tamano: casi O(1) amortizado.
struct DSU {
    std::vector<int> p, sz;
    explicit DSU(int n) : p(n), sz(n, 1) { std::iota(p.begin(), p.end(), 0); }
    int find(int x) { return p[x] == x ? x : p[x] = find(p[x]); }
    bool unite(int a, int b) {
        a = find(a);
        b = find(b);
        if (a == b) return false;
        if (sz[a] < sz[b]) std::swap(a, b);
        p[b] = a;
        sz[a] += sz[b];
        return true;
    }
};

// DFS de Tarjan: disc[u] = orden de descubrimiento; low[u] = menor disc alcanzable
// desde el subarbol de u usando a lo sumo una arista de retorno.
struct Articulations {
    const std::vector<std::vector<int>>& adj;
    std::vector<int> disc, low;
    std::vector<char> isArt;
    int timer = 0;

    explicit Articulations(const std::vector<std::vector<int>>& a)
        : adj(a), disc(a.size(), -1), low(a.size(), 0), isArt(a.size(), 0) {}

    void dfs(int u, int parent) {
        disc[u] = low[u] = timer++;
        int children = 0;
        for (int v : adj[u]) {
            if (v == parent) continue;
            if (disc[v] != -1) {
                low[u] = std::min(low[u], disc[v]);  // arista de retorno
            } else {
                children++;
                dfs(v, u);
                low[u] = std::min(low[u], low[v]);
                if (parent != -1 && low[v] >= disc[u]) isArt[u] = 1;  // v no puede subir por encima de u
            }
        }
        if (parent == -1 && children > 1) isArt[u] = 1;  // raiz con 2 o mas hijos
    }
};

std::string label(int r) { return r == 0 ? "B" : "S" + std::to_string(r); }

}  // namespace

NetworkAnalysis analyzeNetwork(const Scenario& sc, const VisibilityGraph& g) {
    NetworkAnalysis res;
    const int R = 1 + (int)sc.stations.size();
    res.nodes = R;

    // Indices en el grafo de visibilidad: base = 0, clientes 1..k, estaciones k+1..k+m
    const int k = (int)sc.clients.size();
    auto gi = [&](int r) { return r == 0 ? 0 : k + r; };

    std::vector<REdge> edges;
    std::vector<std::vector<int>> adj(R);
    for (int r = 0; r < R; r++) {
        ShortestPaths sp = dijkstra(g, gi(r));
        for (int s = r + 1; s < R; s++) {
            double d = sp.dist[gi(s)];
            if (d == INF || sc.drone.kappa * d > sc.drone.autonomy + EPS) continue;
            edges.push_back({r, s, d});
            adj[r].push_back(s);
            adj[s].push_back(r);
        }
    }
    res.edges = (int)edges.size();

    // RF11: DFS -> componentes y puntos de articulacion
    Articulations art(adj);
    for (int r = 0; r < R; r++)
        if (art.disc[r] == -1) {
            res.components++;
            art.dfs(r, -1);
        }
    for (int r = 0; r < R; r++)
        if (art.isArt[r]) res.articulation.push_back(label(r));
    res.connected = (res.components == 1);

    // RF12: Kruskal (bosque de expansion minima si la red no es conexa)
    std::sort(edges.begin(), edges.end(), [](const REdge& a, const REdge& b) { return a.w < b.w; });
    DSU dsu(R);
    for (const REdge& e : edges)
        if (dsu.unite(e.u, e.v)) {
            res.mst.push_back({label(e.u), label(e.v), e.w});
            res.mstCost += e.w;
        }
    return res;
}

std::string networkJson(const NetworkAnalysis& r) {
    std::ostringstream o;
    o << std::fixed << std::setprecision(3);
    o << "{\"nodes\":" << r.nodes << ",\"edges\":" << r.edges << ",\"components\":" << r.components
      << ",\"connected\":" << (r.connected ? "true" : "false") << ",\"mstCost\":" << r.mstCost
      << ",\"articulation\":[";
    for (size_t i = 0; i < r.articulation.size(); i++) o << (i ? "," : "") << "\"" << r.articulation[i] << "\"";
    o << "],\"mst\":[";
    for (size_t i = 0; i < r.mst.size(); i++)
        o << (i ? "," : "") << "{\"a\":\"" << r.mst[i].a << "\",\"b\":\"" << r.mst[i].b << "\",\"d\":" << r.mst[i].d << "}";
    o << "]}";
    return o.str();
}
