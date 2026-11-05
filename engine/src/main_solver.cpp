#include <iostream>
#include <string>

#include "reachability.hpp"
#include "scenario.hpp"
#include "solver_exact.hpp"
#include "visibility.hpp"

int main(int argc, char* argv[]) {
    bool reachOnly = false;
    const char* path = nullptr;
    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        if (a == "--reach")
            reachOnly = true;
        else
            path = argv[i];
    }
    if (!path) {
        std::cerr << "Uso: main_solver [--reach] <archivo_entrada>\n";
        return 1;
    }

    Scenario sc;
    std::string err;
    if (!readScenarioFile(path, sc, err) || !validateScenario(sc, err)) {
        std::cerr << "Error de entrada: " << err << "\n";
        if (!reachOnly) std::cout << "STATUS INFEASIBLE\n";
        return 1;
    }

    VisibilityGraph g = buildVisibilityGraph(sc);
    ReachReport rep = analyzeReachability(sc, g);

    if (reachOnly) {
        std::cout << reachJson(sc, rep) << "\n";
        return 0;
    }

    std::cerr << "Escenario " << sc.category << ": " << sc.clients.size() << " clientes, "
              << sc.stations.size() << " estaciones, " << sc.polygons.size() << " poligonos\n"
              << "Grafo de visibilidad: " << g.size() << " nodos, " << g.edgeCount() << " aristas\n";

    // Prefiltro: si algun cliente no se puede atender ni recargando, no hay nada que buscar.
    // (El solver actual visita todos los clientes; con opcionales omitibles, solo contaran los obligatorios.)
    bool infeasible = false;
    for (const ClientReach& c : rep.clients) {
        if (!c.reachable) {
            std::cerr << "Cliente C" << c.id << " inalcanzable: radio " << rep.radius
                      << ", punto de recarga mas cercano de la red a " << c.dNear << "\n";
            infeasible = true;
        }
    }
    if (infeasible) {
        std::cout << "STATUS INFEASIBLE\n";
        return 2;
    }

    Solution sol = solveExact(sc, g);
    writeSolution(std::cout, sol);
    return sol.status == "FEASIBLE" ? 0 : 2;
}
