#include <sys/resource.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <cstdlib>

#include "judge.hpp"
#include "reachability.hpp"
#include "scenario.hpp"
#include "solver_exact.hpp"
#include "solver_heur.hpp"
#include "visibility.hpp"

// tiempo maximo por categoria en milisegundos
static double tmaxMs(const std::string& cat) {
    if (cat == "Small") return 1000;
    if (cat == "Medium") return 2000;
    if (cat == "Large") return 5000;
    return 10000;  // Extreme
}

// memoria maxima por categoria en MB (ajustar segun lo medido)
static double memMaxMb(const std::string& cat) {
    if (cat == "Small") return 64;
    if (cat == "Medium") return 128;
    if (cat == "Large") return 256;
    return 512;  // Extreme
}

// pico de memoria del proceso en MB (en Linux ru_maxrss viene en KB)
static double peakMemMb() {
    struct rusage ru;
    getrusage(RUSAGE_SELF, &ru);
    return ru.ru_maxrss / 1024.0;
}

static const char* HEADER =
    "caso,categoria,clientes,estaciones,poligonos,estado,costo,tiempo_ms,algoritmo,memoria_mb,"
    "recargas,parcial,veredicto,motivo,tipo,semilla,t_mejor_ms,nodos_explorados";
    
    // Tipo del caso a partir del nombre: small_col -> colineal; small_01 -> aleatorio
static std::string tipoCaso(const std::string& name) {
    std::string base = name.substr(0, name.find_last_of('.'));
    if (base.size() >= 4) {
        std::string suf = base.substr(base.size() - 4);
        if (suf == "_col") return "colineal";
        if (suf == "_enc") return "encerrado";
        if (suf == "_lim") return "limite";
        if (suf == "_pas") return "pasillo";
    }
    return "aleatorio";
}

// Uso: main_bench [--no-header] <exacto|heur> <caso.txt>
// Un caso y un algoritmo por proceso: asi memoria_mb es el pico real de ese caso.
int main(int argc, char* argv[]) {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

	
    bool header = true;
    int a = 1;
    if (a < argc && std::string(argv[a]) == "--no-header") {
        header = false;
        a++;
    }
    if (argc - a != 2 || (std::string(argv[a]) != "exacto" && std::string(argv[a]) != "heur")) {
        std::cerr << "Uso: main_bench [--no-header] <exacto|heur> <caso.txt>\n";
        return 1;
    }
    const std::string algo = argv[a], path = argv[a + 1];
    if (header) std::cout << HEADER << "\n";

    std::string name = path.substr(path.find_last_of("/\\") + 1);
    auto t0 = std::chrono::steady_clock::now();
    Scenario sc;
    std::string err;
    if (!readScenarioFile(path, sc, err) || !validateScenario(sc, err)) {
        std::replace(err.begin(), err.end(), ',', ';');  // una coma rompe el CSV
        std::cout << name << ",,,,,,,," << algo << ",,,,RE," << err << ",,,,\n";
        return 0;
    }
    VisibilityGraph g = buildVisibilityGraph(sc);

    bool reach = true;
    for (const ClientReach& c : analyzeReachability(sc, g).clients)
        if (!c.reachable) reach = false;

    // El limite del solver es lo que queda del presupuesto: lectura, grafo y alcanzabilidad ya gastaron tiempo.
    double pre = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    double limit = std::max(50.0, 0.9 * tmaxMs(sc.category) - pre);
    Solution sol;
    double exactLimit = limit;  // presupuesto real que recibe el exacto
    long long tBest = 0;        // instante (ms desde el inicio del solver) de la mejor ruta
    long long nodes = -1;       // solo el exacto lo reporta
     if (reach) {
        if (algo == "exacto") {
            // Ítem 1: el heuristico corre primero con una parte del presupuesto y su ruta
            // se le pasa al exacto como cota superior inicial.
            const char* fe = std::getenv("FRAC_HEUR");
			double frac = fe ? std::atof(fe) : 0.6;
			Solution h = solveHeuristic(sc, g, std::max(20.0, frac * limit));
            exactLimit = std::max(50.0, limit - (double)h.timeMs);
            sol = solveExact(sc, g, exactLimit, &h);
            // Si el B&B mejoro la ruta, su instante se suma al tiempo que ya gasto el heuristico.
            // Si no, la mejor ruta sigue siendo la del heuristico.
            bool improved = (h.status != "FEASIBLE") || (sol.cost < h.cost - 1e-9);
            tBest = improved ? h.timeMs + sol.timeBestMs : h.timeBestMs;
            nodes = sol.nodes;
        } else {
            sol = solveHeuristic(sc, g, limit);
            tBest = sol.timeBestMs;
        }
    }
    double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    double mem = peakMemMb();

    std::string status = reach ? sol.status : "INFEASIBLE";
    // "parcial" solo tiene sentido para el exacto: se agoto el tiempo sin probar optimalidad.
    // El heuristico no promete optimo, asi que se marca NA.
    std::string partial = algo == "heur" ? "NA" : ((reach && sol.timeMs >= (long long)exactLimit) ? "SI" : "NO");
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
            std::replace(reason.begin(), reason.end(), ',', ';');
        } else if (ms > tmaxMs(sc.category)) {
            verdict = "TLE";
            reason = "excede el tiempo maximo";
        } else if (mem > memMaxMb(sc.category)) {
            verdict = "MLE";
            reason = "excede la memoria maxima";
        } else {
            verdict = "AC";
            if (partial == "SI") reason = "tiempo agotado; ruta valida sin garantia de optimo";
        }
    }

    std::cout << name << "," << sc.category << "," << sc.clients.size() << "," << sc.stations.size() << ","
              << sc.polygons.size() << "," << status << ",";
    if (status == "FEASIBLE") std::cout << std::fixed << std::setprecision(3) << sol.cost;
    std::cout << "," << std::llround(ms) << "," << algo << "," << std::fixed << std::setprecision(1) << mem << ",";
    if (status == "FEASIBLE") std::cout << sol.recharges;
    std::cout << "," << partial << "," << verdict << "," << reason << "," << tipoCaso(name) << "," << sc.seed << ",";
    if (status == "FEASIBLE") std::cout << tBest;
    std::cout << ",";
    if (nodes >= 0) std::cout << nodes;
    std::cout << "\n";
    return 0;
}
