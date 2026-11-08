#include <fstream>
#include <iostream>
#include <string>

#include "scenario.hpp"
#include "station_planner.hpp"

int main(int argc, char* argv[]) {
    PlannerOptions opt;
    std::string scenarioPath, districtPath, outPath;
    bool hasA = false;
    double overrideA = 0;
    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        if (a == "--step" && i + 1 < argc)
            opt.step = std::stod(argv[++i]);
        else if (a == "--service" && i + 1 < argc)
            opt.serviceSeconds = std::stod(argv[++i]);
        else if (a == "--out" && i + 1 < argc)
            outPath = argv[++i];
        else if (a == "--A" && i + 1 < argc) {
            overrideA = std::stod(argv[++i]);
            hasA = true;
        }
        else if (scenarioPath.empty())
            scenarioPath = a;
        else
            districtPath = a;
    }
    if (scenarioPath.empty() || districtPath.empty()) {
        std::cerr << "Uso: main_planner <escenario.txt> <contorno.txt> [--A autonomia] [--step 250] [--service 30] [--out salida.txt]\n";
        return 1;
    }

    Scenario sc;
    std::string err;
    if (!readScenarioFile(scenarioPath, sc, err) || !validateScenario(sc, err)) {
        std::cerr << "Error de entrada: " << err << "\n";
        return 1;
    }
    if (hasA) sc.drone.autonomy = overrideA;
    std::vector<Point> district;
    if (!readDistrict(districtPath, district, err)) {
        std::cerr << "Error en el contorno: " << err << "\n";
        return 1;
    }

    PlanResult r = planStations(sc, district, opt);
    std::cerr << "Radio de cobertura: " << r.radius << " m | malla: " << r.grid.size() << " puntos (cada " << opt.step
              << " m)\n"
              << "Estaciones nuevas: " << r.stations.size() << " | puntos cubiertos: " << r.covered << " de "
              << r.grid.size() << " | sin cubrir: " << r.uncovered.size() << "\n";

    if (!outPath.empty()) {
        std::ofstream out(outPath);
        if (!out) {
            std::cerr << "No se pudo escribir: " << outPath << "\n";
            return 1;
        }
        writeScenario(out, withStations(sc, r));
        std::cerr << "Escenario con estaciones: " << outPath << "\n";
    }
    std::cout << planJson(r) << "\n";
    return 0;
}
