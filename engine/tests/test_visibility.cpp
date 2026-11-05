#include <cmath>
#include <iostream>
#include <sstream>

#include "visibility.hpp"

static int fails = 0;
#define CHECK(cond)                                                     \
    do {                                                                \
        if (!(cond)) {                                                  \
            std::cout << "FALLO linea " << __LINE__ << ": " #cond "\n"; \
            fails++;                                                    \
        }                                                               \
    } while (0)

static const char* EJEMPLO =
    "SEED 20261\nCATEGORY Small\n2 1 1\n"
    "A 20 KAPPA 1 V 1 Q 10 TAU 0 KH 0\n"
    "BASE 1 4\n"
    "CLIENT 1 11 4 2 1 0 1000 0 1\n"
    "CLIENT 2 6 9 1 1 0 1000 0 1\n"
    "STATION 1 9 0\n"
    "POLYGON 1 4\n4 2\n8 2\n8 6\n4 6\n";

int main() {
    Scenario sc;
    std::string err;
    std::istringstream in(EJEMPLO);
    CHECK(readScenario(in, sc, err));

    VisibilityGraph g = buildVisibilityGraph(sc);
    CHECK(g.size() == 8);
    CHECK(g.edgeCount() == 16);  // PT08: 16 aristas de 28 posibles

    int B = g.indexOf("B"), C1 = g.indexOf("C1"), C2 = g.indexOf("C2"), S1 = g.indexOf("S1");
    int W1 = g.indexOf("W1.1"), W2 = g.indexOf("W1.2"), W3 = g.indexOf("W1.3"), W4 = g.indexOf("W1.4");
    CHECK(B >= 0 && C1 >= 0 && C2 >= 0 && S1 >= 0 && W1 >= 0 && W2 >= 0 && W3 >= 0 && W4 >= 0);

    CHECK(!g.hasEdge(B, C1));   // el poligono lo bloquea
    CHECK(g.hasEdge(B, C2));
    CHECK(g.hasEdge(B, W1));    // vertice (4,2)
    CHECK(g.hasEdge(B, W4));    // vertice (4,6)
    CHECK(!g.hasEdge(B, W2));   // entraria al interior
    CHECK(g.hasEdge(C1, C2));
    CHECK(g.hasEdge(C1, S1));
    CHECK(!g.hasEdge(C2, S1));  // atraviesa el poligono
    CHECK(g.hasEdge(W1, W2));   // volar a lo largo de una arista es valido
    CHECK(!g.hasEdge(W1, W3));  // diagonales: interior
    CHECK(!g.hasEdge(W2, W4));

    // Simetria
    for (int u = 0; u < g.size(); u++)
        for (const Edge& e : g.adj[u]) CHECK(g.hasEdge(e.to, u));

    // Peso de B-W1.1 = sqrt(13)
    for (const Edge& e : g.adj[B])
        if (e.to == W1) CHECK(std::fabs(e.w - std::sqrt(13.0)) < 1e-9);

    // Sin obstaculos el grafo es completo
    Scenario libre;
    std::istringstream in2(
        "SEED 1\nCATEGORY Small\n1 0 0\nA 20 KAPPA 1 V 1 Q 10 TAU 0 KH 0\n"
        "BASE 1 1\nCLIENT 1 5 5 1 1 0 1000 0 1\n");
    CHECK(readScenario(in2, libre, err));
    VisibilityGraph g2 = buildVisibilityGraph(libre);
    CHECK(g2.size() == 2 && g2.edgeCount() == 1);

    if (fails == 0) std::cout << "OK: todas las pruebas del grafo de visibilidad pasaron\n";
    return fails == 0 ? 0 : 1;
}
