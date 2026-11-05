#include <iostream>

#include "scenario.hpp"
#include "solver_exact.hpp"
#include "visibility.hpp"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Uso: main_solver <archivo_entrada>\n";
        return 1;
    }

    Scenario sc;
    std::string err;
    if (!readScenarioFile(argv[1], sc, err) || !validateScenario(sc, err)) {
        std::cerr << "Error de entrada: " << err << "\n";
        std::cout << "STATUS INFEASIBLE\n";
        return 1;
    }

    VisibilityGraph g = buildVisibilityGraph(sc);
    std::cerr << "Escenario " << sc.category << ": " << sc.clients.size() << " clientes, "
              << sc.stations.size() << " estaciones, " << sc.polygons.size() << " poligonos\n"
              << "Grafo de visibilidad: " << g.size() << " nodos, " << g.edgeCount() << " aristas\n";

    Solution sol = solveExact(sc, g);
    writeSolution(std::cout, sol);
    return sol.status == "FEASIBLE" ? 0 : 2;
}
