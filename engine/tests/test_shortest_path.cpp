#include <cmath>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>

#include "shortest_path.hpp"

static int fails = 0;
#define CHECK(cond)                                                     \
    do {                                                                \
        if (!(cond)) {                                                  \
            std::cout << "FALLO linea " << __LINE__ << ": " #cond "\n"; \
            fails++;                                                    \
        }                                                               \
    } while (0)

static const double INF = std::numeric_limits<double>::infinity();

static const char* EJEMPLO =
    "SEED 20261\nCATEGORY Small\n2 1 1\n"
    "A 20 KAPPA 1 V 1 Q 10 TAU 0 KH 0\n"
    "BASE 1 4\n"
    "CLIENT 1 11 4 2 1 0 1000 0 1\n"
    "CLIENT 2 6 9 1 1 0 1000 0 1\n"
    "STATION 1 9 0\n"
    "POLYGON 1 4\n4 2\n8 2\n8 6\n4 6\n";

static VisibilityGraph randomGraph(std::mt19937_64& rng, int n, double p) {
    VisibilityGraph g;
    g.nodes.assign(n, Node{});
    g.adj.assign(n, std::vector<Edge>());
    std::uniform_real_distribution<double> U(0.0, 1.0), W(0.5, 100.0);
    for (int i = 0; i < n; i++)
        for (int j = i + 1; j < n; j++)
            if (U(rng) < p) {
                double w = W(rng);
                g.adj[i].push_back({j, w});
                g.adj[j].push_back({i, w});
            }
    return g;
}

static double edgeWeight(const VisibilityGraph& g, int u, int v) {
    for (const Edge& e : g.adj[u])
        if (e.to == v) return e.w;
    return INF;
}

int main() {
    // --- Escenario de la seccion 3.2 ---
    Scenario sc;
    std::string err;
    std::istringstream in(EJEMPLO);
    CHECK(readScenario(in, sc, err));
    VisibilityGraph g = buildVisibilityGraph(sc);

    int B = g.indexOf("B"), C1 = g.indexOf("C1"), C2 = g.indexOf("C2"), S1 = g.indexOf("S1");
    int W1 = g.indexOf("W1.1");

    ShortestPaths fromB = dijkstra(g, B);
    CHECK(std::fabs(fromB.dist[C1] - 11.211) < 1e-3);  // PT09
    CHECK(std::fabs(fromB.dist[C1] - (2 * std::sqrt(13.0) + 4)) < 1e-9);
    CHECK(std::fabs(fromB.dist[C2] - std::sqrt(50.0)) < 1e-9);
    CHECK(std::fabs(fromB.dist[S1] - (std::sqrt(13.0) + std::sqrt(29.0))) < 1e-9);

    // B -> C1: hay dos caminos optimos empatados (por abajo o por arriba del cuadrado)
    auto labels = pathLabels(g, reconstructPath(fromB, B, C1));
    CHECK(labels.size() == 4 && labels.front() == "B" && labels.back() == "C1");
    bool abajo = labels.size() == 4 && labels[1] == "W1.1" && labels[2] == "W1.2";
    bool arriba = labels.size() == 4 && labels[1] == "W1.4" && labels[2] == "W1.3";
    CHECK(abajo || arriba);

    CHECK(reconstructPath(fromB, B, B) == std::vector<int>{B});

    // Costo de la ruta de referencia B C2 C1 S1 W1.1 B = 27.605
    std::vector<int> ruta = {B, C2, C1, S1, W1, B};
    double total = 0;
    for (size_t i = 0; i + 1 < ruta.size(); i++) total += dijkstra(g, ruta[i]).dist[ruta[i + 1]];
    CHECK(std::fabs(total - 27.605) < 1e-3);

    // Bateria restante tras C1: A - (d(B,C2) + d(C2,C1)) = 5.86
    double tras = 20.0 - (fromB.dist[C2] + dijkstra(g, C2).dist[C1]);
    CHECK(std::fabs(tras - 5.858) < 1e-3);

    // --- Dijkstra == Floyd-Warshall en grafos aleatorios (n <= 100) ---
    std::mt19937_64 rng(20261);
    const double dens[3] = {0.03, 0.1, 0.5};  // incluye grafos desconexos
    for (int caso = 0; caso < 30; caso++) {
        int n = 2 + (int)(rng() % 99);
        VisibilityGraph r = randomGraph(rng, n, dens[caso % 3]);
        auto fw = floydWarshall(r);
        for (int s = 0; s < n; s++) {
            ShortestPaths sp = dijkstra(r, s);
            for (int t = 0; t < n; t++) {
                bool ok = (sp.dist[t] == INF && fw[s][t] == INF) || std::fabs(sp.dist[t] - fw[s][t]) < 1e-9;
                CHECK(ok);
                // la ruta reconstruida suma exactamente la distancia
                std::vector<int> p = reconstructPath(sp, s, t);
                if (sp.dist[t] == INF) {
                    CHECK(p.empty());
                } else {
                    CHECK(!p.empty() && p.front() == s && p.back() == t);
                    double suma = 0;
                    for (size_t i = 0; i + 1 < p.size(); i++) suma += edgeWeight(r, p[i], p[i + 1]);
                    CHECK(std::fabs(suma - sp.dist[t]) < 1e-9);
                }
            }
        }
    }

    // --- Grafo desconexo: nodo aislado ---
    VisibilityGraph aislado;
    aislado.nodes.assign(3, Node{});
    aislado.adj.assign(3, std::vector<Edge>());
    aislado.adj[0].push_back({1, 2.0});
    aislado.adj[1].push_back({0, 2.0});
    ShortestPaths sa = dijkstra(aislado, 0);
    CHECK(sa.dist[1] == 2.0 && sa.dist[2] == INF);
    CHECK(reconstructPath(sa, 0, 2).empty());

    if (fails == 0) std::cout << "OK: todas las pruebas de caminos minimos pasaron\n";
    return fails == 0 ? 0 : 1;
}
