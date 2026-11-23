#include <iomanip>
#include <iostream>
#include <string>

#include "graph_analysis.hpp"
#include "scenario.hpp"
#include "visibility.hpp"

// Uso: main_network <caso1.txt> [caso2.txt ...]   -> CSV por caso (RF11 y RF12)
// Sirve para comparar el costo de infraestructura (MST) entre escenarios.
int main(int argc, char* argv[]) {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    if (argc < 2) {
        std::cerr << "Uso: main_network <caso1.txt> [caso2.txt ...]\n";
        return 1;
    }
    std::cout << "caso,categoria,estaciones,nodos_red,aristas_red,componentes,conexa,"
                 "n_articulaciones,articulaciones,costo_mst\n";
    for (int i = 1; i < argc; i++) {
        std::string path = argv[i], err;
        std::string name = path.substr(path.find_last_of("/\\") + 1);
        Scenario sc;
        if (!readScenarioFile(path, sc, err) || !validateScenario(sc, err)) {
            std::cerr << name << ": " << err << "\n";
            continue;
        }
        VisibilityGraph g = buildVisibilityGraph(sc);
        NetworkAnalysis r = analyzeNetwork(sc, g);
        std::string arts;
        for (size_t t = 0; t < r.articulation.size(); t++) arts += (t ? " " : "") + r.articulation[t];
        std::cout << name << "," << sc.category << "," << sc.stations.size() << "," << r.nodes << "," << r.edges
                  << "," << r.components << "," << (r.connected ? "SI" : "NO") << "," << r.articulation.size()
                  << "," << arts << "," << std::fixed << std::setprecision(3) << r.mstCost << "\n";
    }
    return 0;
}
