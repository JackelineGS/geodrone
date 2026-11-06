#include <cmath>
#include <iostream>
#include <random>
#include <sstream>

#include "param_search.hpp"
#include "solver_exact.hpp"

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

static Scenario load(const std::string& text) {
    Scenario sc;
    std::string err;
    std::istringstream in(text);
    bool ok = readScenario(in, sc, err);
    CHECK(ok);
    return sc;
}

static std::string randomScenario(std::mt19937_64& rng, int k, int m, double A) {
    auto R = [&](int lo, int hi) { return lo + (int)(rng() % (unsigned)(hi - lo + 1)); };
    std::ostringstream s;
    s << "SEED 1\nCATEGORY Small\n" << k << " " << m << " 0\n"
      << "A " << A << " KAPPA 1 V 1 Q 10 TAU 0 KH 0\n"
      << "BASE " << R(0, 50) << " " << R(0, 50) << "\n";
    for (int i = 1; i <= k; i++) s << "CLIENT " << i << " " << R(0, 50) << " " << R(0, 50) << " 1 1 0 1000 0 1\n";
    for (int i = 1; i <= m; i++) s << "STATION " << i << " " << R(0, 50) << " " << R(0, 50) << "\n";
    return s.str();
}

int main() {
    // --- Ejemplo de la seccion 3.2: A* = 2 * d(B,C2) = 10*sqrt(2) ---
    {
        Scenario sc = load(EJEMPLO);
        VisibilityGraph g = buildVisibilityGraph(sc);
        MinAutonomy r = minAutonomy(sc, g);
        CHECK(r.feasible);
        CHECK(std::fabs(r.value - 10 * std::sqrt(2.0)) < 1e-5);  // converge a A (+-eps)
        CHECK(r.iterations > 0 && r.iterations < 100);

        // A igual al minimo: factible; A - eps: infactible (PT12)
        CHECK(missionFeasible(sc, g, r.value + 1e-3));
        CHECK(!missionFeasible(sc, g, r.value - 1e-3));

        // Lo mismo con el solver exacto, que es la referencia
        Scenario a = sc;
        a.drone.autonomy = r.value + 1e-3;
        CHECK(solveExact(a, g).status == "FEASIBLE");
        a.drone.autonomy = r.value - 1e-3;
        CHECK(solveExact(a, g).status == "INFEASIBLE");

        std::string js = minAutonomyJson(sc, r);
        CHECK(js.find("\"currentFeasible\":true") != std::string::npos);
    }

    // --- Un cliente a distancia 10, servicio 30 s con kh = 0.2: A* = 2*10 + 0.2*30 = 26 ---
    {
        Scenario sc = load(
            "SEED 1\nCATEGORY Small\n1 0 0\nA 100 KAPPA 1 V 1 Q 10 TAU 0 KH 0.2\n"
            "BASE 0 0\nCLIENT 1 10 0 1 1 0 1000 30 1\n");
        VisibilityGraph g = buildVisibilityGraph(sc);
        MinAutonomy r = minAutonomy(sc, g);
        CHECK(r.feasible && std::fabs(r.value - 26.0) < 1e-5);
    }

    // --- Sin clientes: no hace falta bateria ---
    {
        Scenario sc = load("SEED 1\nCATEGORY Small\n0 0 0\nA 10 KAPPA 1 V 1 Q 10 TAU 0 KH 0\nBASE 0 0\n");
        VisibilityGraph g = buildVisibilityGraph(sc);
        MinAutonomy r = minAutonomy(sc, g);
        CHECK(r.feasible && r.value == 0);
    }

    // --- Aleatorios: monotonia y coherencia con el solver exacto ---
    std::mt19937_64 rng(20261);
    for (int caso = 0; caso < 25; caso++) {
        Scenario sc = load(randomScenario(rng, 2 + caso % 4, caso % 3, 50));
        VisibilityGraph g = buildVisibilityGraph(sc);
        MinAutonomy r = minAutonomy(sc, g);
        CHECK(r.feasible);  // sin obstaculos nadie queda encerrado

        // monotonia: una vez factible, sigue factible al subir A
        bool vioFactible = false;
        for (int s = 0; s <= 40; s++) {
            bool f = missionFeasible(sc, g, s * 2.5);
            if (vioFactible) CHECK(f);
            if (f) vioFactible = true;
        }

        // el minimo es factible para el solver exacto, y un poco menos no lo es
        Scenario a = sc;
        a.drone.autonomy = r.value + 1e-3;
        CHECK(solveExact(a, g).status == "FEASIBLE");
        if (r.value > 1e-3) {
            a.drone.autonomy = r.value - 1e-3;
            CHECK(solveExact(a, g).status == "INFEASIBLE");
        }
    }

    if (fails == 0) std::cout << "OK: todas las pruebas de busqueda binaria de autonomia pasaron\n";
    return fails == 0 ? 0 : 1;
}
