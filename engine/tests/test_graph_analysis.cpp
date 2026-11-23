#include <cmath>
#include <iostream>

#include "graph_analysis.hpp"
#include "scenario.hpp"
#include "visibility.hpp"

static int fails = 0;
#define CHECK(c)                                                    \
    do {                                                            \
        if (!(c)) { std::cerr << "FALLA linea " << __LINE__ << ": " #c "\n"; fails++; } \
    } while (0)

// Base en (0,0), tres estaciones en linea: B -- S1 -- S2 -- S3, cada salto de 10 y autonomia 15 (kappa=1)
// => red en cadena: B-S1, S1-S2, S2-S3. Articulaciones: S1 y S2. MST = 30.
int main() {
    Scenario sc;
    sc.drone.autonomy = 15;
    sc.drone.kappa = 1;
    sc.base = {0, 0};
    sc.stations = {{1, {10, 0}}, {2, {20, 0}}, {3, {30, 0}}};
    VisibilityGraph g = buildVisibilityGraph(sc);
    NetworkAnalysis r = analyzeNetwork(sc, g);
    CHECK(r.nodes == 4);
    CHECK(r.edges == 3);
    CHECK(r.components == 1 && r.connected);
    CHECK(r.articulation.size() == 2);
    CHECK(std::fabs(r.mstCost - 30.0) < 1e-9);

    // Con autonomia 25 aparecen B-S2 y S1-S3: la red tiene ciclos y ninguna estacion es articulacion
    sc.drone.autonomy = 25;
    g = buildVisibilityGraph(sc);
    r = analyzeNetwork(sc, g);
    CHECK(r.edges == 5);               // 3 saltos de 10 + B-S2 (20) + S1-S3 (20)
    CHECK(std::fabs(r.mstCost - 30.0) < 1e-9);
    CHECK(r.articulation.empty());
    // Autonomia muy chica: red desconectada
    sc.drone.autonomy = 5;
    g = buildVisibilityGraph(sc);
    r = analyzeNetwork(sc, g);
    CHECK(r.components == 4 && !r.connected && r.edges == 0);

    if (fails == 0) std::cout << "OK test_graph_analysis\n";
    return fails;
}
