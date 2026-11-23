#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include "judge.hpp"
#include "solver_exact.hpp"
#include "solver_heur.hpp"
#include "visibility.hpp"

static int fails = 0;
#define CHECK(cond)                                                     \
    do {                                                                \
        if (!(cond)) {                                                  \
            std::cout << "FALLO linea " << __LINE__ << ": " #cond "\n"; \
            fails++;                                                    \
        }                                                               \
    } while (0)

// Uso: test_solver_heur  (desde la raiz del proyecto)
// Compara el heuristico con el exacto en los casos Small factibles.
int main() {
    const char* files[] = {"small_01", "small_02", "small_03", "small_04", "small_05",
                           "small_col", "small_lim", "small_pas", "small_enc"};
    for (const char* f : files) {
        Scenario sc;
        std::string err, path = std::string("data/gen/") + f + ".txt";
        if (!readScenarioFile(path, sc, err)) {
            std::cout << "no se pudo leer " << path << "\n";
            fails++;
            continue;
        }
        VisibilityGraph g = buildVisibilityGraph(sc);
        Solution ex = solveExact(sc, g, 900);
        Solution he = solveHeuristic(sc, g, 300);
        if (ex.status != "FEASIBLE") {
            CHECK(he.status != "FEASIBLE");  // si el exacto dice infactible, el heuristico tambien
            std::cout << f << ": infactible en ambos\n";
            continue;
        }
        CHECK(he.status == "FEASIBLE");
        JudgeResult jr = judgeSolution(sc, g, he);
        CHECK(jr.ok);                           // la ruta pasa el juez independiente
        CHECK(he.cost >= ex.cost - 1e-6);       // no puede ser mejor que el optimo
        double gap = 100.0 * (he.cost - ex.cost) / ex.cost;
        std::cout << f << ": exacto " << ex.cost << "  heuristico " << he.cost << "  gap " << gap << "%  juez "
                  << jr.reason << "\n";
    }
    std::cout << (fails ? "HAY FALLOS\n" : "OK\n");
    return fails ? 1 : 0;
}
