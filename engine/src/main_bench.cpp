#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>

#include "judge.hpp"
#include "reachability.hpp"
#include "scenario.hpp"
#include "solver_exact.hpp"
#include "visibility.hpp"

// tiempo maximo por categoria en milisegundos
static double tmaxMs(const std::string& cat) {
    if (cat == "Small") return 1000;
    if (cat == "Medium") return 2000;
    if (cat == "Large") return 5000;
    return 10000;  // Extreme
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Uso: main_bench <caso1.txt> [caso2.txt ...]\n";
        return 1;
    }
    std::cout << "caso,categoria,clientes,estaciones,poligonos,estado,costo,tiempo_ms,"
                 "recargas,parcial,veredicto,motivo\n";
    int ac = 0, total = 0;

    for (int a = 1; a < argc; a++) {
        std::string path = argv[a];
        total++;
        //nombre
        std::string name = path.substr(path.find_last_of("/\\") + 1);

        auto t0 = std::chrono::steady_clock::now();
        Scenario sc;
        std::string err;
        if (!readScenarioFile(path, sc, err) || !validateScenario(sc, err)) {
            std::cout << name << ",,,,,,,,,,RE," << err << "\n";
            continue;
        }
        VisibilityGraph g = buildVisibilityGraph(sc);

        bool reach = true;
        for (const ClientReach& c : analyzeReachability(sc, g).clients)
            if (!c.reachable) reach = false;

        double limit = 0.9 * tmaxMs(sc.category);
        Solution sol;
        if (reach) sol = solveExact(sc, g, limit);
        double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();

        std::string status = reach ? sol.status : "INFEASIBLE";
        bool partial = reach && sol.timeMs >= (long long)limit;
        std::string verdict, reason;

        if (!reach) {
            verdict = "AC";
            reason = "infactible correcto: hay un cliente inalcanzable";
        } else if (sol.status != "FEASIBLE") {
            verdict = "WA";
            reason = "el solver no hallo ruta pero todos los clientes son alcanzables";
        } else {
            JudgeResult jr = judgeSolution(sc, g, sol);
            if (!jr.ok) {
                verdict = "WA";
                reason = jr.reason;
            } else if (ms > tmaxMs(sc.category)) {
                verdict = "TLE";
                reason = "excede el tiempo maximo";
            } else {
                verdict = "AC";
                if (partial) reason = "tiempo agotado; ruta valida sin garantia de optimo";
            }
        }
        if (verdict == "AC") ac++;

        std::cout << name << "," << sc.category << "," << sc.clients.size() << "," << sc.stations.size() << ","
                  << sc.polygons.size() << "," << status << ",";
        if (status == "FEASIBLE") std::cout << std::fixed << std::setprecision(3) << sol.cost;
        std::cout << "," << std::llround(ms) << ",";
        if (status == "FEASIBLE") std::cout << sol.recharges;
        std::cout << "," << (partial ? "SI" : "NO") << "," << verdict << "," << reason << "\n";
    }
    std::cerr << "AC: " << ac << " de " << total << " casos\n";
    return 0;
}