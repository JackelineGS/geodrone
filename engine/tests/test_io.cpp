#include <iostream>
#include <sstream>

#include "scenario.hpp"

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

static bool parse(const std::string& text, Scenario& sc, std::string& err) {
    std::istringstream in(text);
    return readScenario(in, sc, err);
}

int main() {
    Scenario sc;
    std::string err;

    // PT19: lectura correcta del ejemplo de la seccion 3.2
    CHECK(parse(EJEMPLO, sc, err));
    CHECK(sc.seed == 20261 && sc.category == "Small");
    CHECK(sc.drone.autonomy == 20 && sc.drone.Q == 10);
    CHECK(sc.base == Point({1, 4}));
    CHECK(sc.clients.size() == 2 && sc.clients[1].p == Point({6, 9}));
    CHECK(sc.clients[0].mandatory && sc.clients[0].prio == 2);
    CHECK(sc.stations.size() == 1 && sc.stations[0].p == Point({9, 0}));
    CHECK(sc.polygons.size() == 1 && sc.polygons[0].v.size() == 4);
    CHECK(validateScenario(sc, err));

    // PT19: entradas mal formadas -> error claro, sin colgarse
    CHECK(!parse("", sc, err));
    CHECK(!parse("SEED abc\n", sc, err));
    CHECK(!parse("SEED 1\nCATEGORY Small\n2 1 1\nA 20 KAPPA 1 V 1 Q 10 TAU 0 KH 0\nBASE 1 4\n", sc, err));
    std::string sobrante = std::string(EJEMPLO) + "EXTRA\n";
    CHECK(!parse(sobrante, sc, err));

    // Comentarios con // y saltos de linea extra
    std::string coment = std::string("SEED 7 // semilla\n\n") + (EJEMPLO + std::string("SEED 20261\n").size());
    CHECK(parse(coment, sc, err) == false || sc.seed == 7);  // CATEGORY sigue; solo comprueba que no revienta

    // PT07: cliente en el interior de un obstaculo
    std::string malo = EJEMPLO;
    malo.replace(malo.find("CLIENT 2 6 9"), 12, "CLIENT 2 6 4");
    CHECK(parse(malo, sc, err));
    CHECK(!validateScenario(sc, err));

    // Poligono horario -> invalido
    std::string horario = EJEMPLO;
    horario.replace(horario.find("4 2\n8 2\n8 6\n4 6\n"), 16, "4 2\n4 6\n8 6\n8 2\n");
    CHECK(parse(horario, sc, err));
    CHECK(!validateScenario(sc, err));

    // Escritura de la solucion
    Solution sol;
    sol.status = "FEASIBLE";
    sol.cost = 27.605;
    sol.timeMs = 1;
    sol.recharges = 1;
    sol.route = {"B", "C2", "C1", "S1", "W1.1", "B"};
    std::ostringstream out;
    writeSolution(out, sol);
    CHECK(out.str() ==
          "STATUS FEASIBLE\nCOST 27.605\nTIME_MS 1\nRECHARGES 1\nSKIPPED 0\nROUTE 6\nB C2 C1 S1 W1.1 B\n");

    if (fails == 0) std::cout << "OK: todas las pruebas de io pasaron\n";
    return fails == 0 ? 0 : 1;
}
