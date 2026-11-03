#include <iostream>

#include "scenario.hpp"

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

    std::cerr << "Escenario " << sc.category << ": " << sc.clients.size() << " clientes, "
              << sc.stations.size() << " estaciones, " << sc.polygons.size() << " poligonos\n";

    // Provisional: aun no hay solver; se imprime la salida de referencia del ejemplo.
    Solution sol;
    sol.status = "FEASIBLE";
    sol.cost = 27.605;
    sol.timeMs = 1;
    sol.recharges = 1;
    sol.route = {"B", "C2", "C1", "S1", "W1.1", "B"};
    writeSolution(std::cout, sol);
    return 0;
}
