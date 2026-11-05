#include <algorithm>
#include <cmath>
#include <iostream>
#include <numeric>
#include <random>
#include <sstream>

#include "shortest_path.hpp"
#include "solver_exact.hpp"
#include "visibility.hpp"

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

// Mini juez: ruta B ... B, bateria nunca negativa, sin entrar a obstaculos, cada cliente una vez.
static bool judge(const Scenario& sc, const VisibilityGraph& g, const std::vector<std::string>& route, double& cost) {
    if (route.size() < 2 || route.front() != "B" || route.back() != "B") return false;
    double battery = sc.drone.autonomy;
    cost = 0;
    std::vector<int> seen(sc.clients.size() + 1, 0);
    for (size_t i = 1; i < route.size(); i++) {
        int ia = g.indexOf(route[i - 1]), ib = g.indexOf(route[i]);
        if (ia < 0 || ib < 0) return false;
        Point a = g.nodes[ia].p, b = g.nodes[ib].p;
        for (const Polygon& poly : sc.polygons)
            if (!segmentValid(poly, a, b)) return false;
        double d = dist(a, b);
        cost += d;
        battery -= sc.drone.kappa * d;
        const std::string& l = route[i];
        if (l[0] == 'C') {
            int id = std::stoi(l.substr(1));
            for (const Client& c : sc.clients)
                if (c.id == id) battery -= sc.drone.kh * (double)c.s;
            if (id < 1 || id >= (int)seen.size()) return false;
            seen[id]++;
        }
        if (battery < -1e-9) return false;
        if (l[0] == 'S' || l == "B") battery = sc.drone.autonomy;
    }
    for (size_t id = 1; id < seen.size(); id++)
        if (seen[id] != 1) return false;
    return true;
}

// TSP por fuerza bruta (distancia euclidiana, sin obstaculos ni bateria)
static double bruteTsp(const Scenario& sc) {
    int k = (int)sc.clients.size();
    std::vector<int> perm(k);
    std::iota(perm.begin(), perm.end(), 0);
    double best = 1e18;
    do {
        double len = 0;
        Point cur = sc.base;
        for (int j : perm) {
            len += dist(cur, sc.clients[j].p);
            cur = sc.clients[j].p;
        }
        len += dist(cur, sc.base);
        best = std::min(best, len);
    } while (std::next_permutation(perm.begin(), perm.end()));
    return best;
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
    // --- Ejemplo de la seccion 3.2: debe reproducir la ruta de referencia ---
    {
        Scenario sc = load(EJEMPLO);
        VisibilityGraph g = buildVisibilityGraph(sc);
        Solution sol = solveExact(sc, g);
        CHECK(sol.status == "FEASIBLE");
        CHECK(std::fabs(sol.cost - 27.605) < 1e-3);
        CHECK(sol.recharges == 1 && sol.skipped == 0);
        CHECK(sol.routes.size() == 1);
        std::vector<std::string> esperada = {"B", "C2", "C1", "S1", "W1.1", "B"};
        CHECK(sol.routes.size() == 1 && sol.routes[0] == esperada);
        double c = 0;
        CHECK(sol.routes.size() == 1 && judge(sc, g, sol.routes[0], c));
        CHECK(std::fabs(c - sol.cost) < 1e-6);
    }

    // --- Con mas bateria ya no hace falta recargar: B C2 C1 B (o su inverso) ---
    {
        std::string t = EJEMPLO;
        t.replace(t.find("A 20"), 4, "A 40");
        Scenario sc = load(t);
        VisibilityGraph g = buildVisibilityGraph(sc);
        Solution sol = solveExact(sc, g);
        CHECK(sol.status == "FEASIBLE" && sol.recharges == 0);
        double c = 0;
        CHECK(sol.routes.size() == 1 && judge(sc, g, sol.routes[0], c));
        CHECK(sol.cost < 27.605);
    }

    // --- PT11: A menor que el viaje de ida y vuelta, sin estaciones -> INFEASIBLE ---
    {
        Scenario sc = load(
            "SEED 1\nCATEGORY Small\n1 0 0\nA 5 KAPPA 1 V 1 Q 10 TAU 0 KH 0\n"
            "BASE 1 1\nCLIENT 1 11 1 1 1 0 1000 0 1\n");
        VisibilityGraph g = buildVisibilityGraph(sc);
        Solution sol = solveExact(sc, g);
        CHECK(sol.status == "INFEASIBLE");
    }

    // --- Sin limite de bateria: igual al TSP por fuerza bruta ---
    std::mt19937_64 rng(20261);
    for (int caso = 0; caso < 20; caso++) {
        int k = 3 + caso % 5;
        Scenario sc = load(randomScenario(rng, k, 0, 1e9));
        VisibilityGraph g = buildVisibilityGraph(sc);
        Solution sol = solveExact(sc, g);
        CHECK(sol.status == "FEASIBLE");
        CHECK(std::fabs(sol.cost - bruteTsp(sc)) < 1e-6);
        double c = 0;
        CHECK(!sol.routes.empty() && judge(sc, g, sol.routes[0], c));
        CHECK(std::fabs(c - sol.cost) < 1e-6);
    }

    // --- Con bateria limitada y estaciones: la ruta debe ser valida y no mejor que el TSP ---
    int factibles = 0;
    for (int caso = 0; caso < 20; caso++) {
        int k = 3 + caso % 4;
        Scenario sc = load(randomScenario(rng, k, 2, 90));
        VisibilityGraph g = buildVisibilityGraph(sc);
        Solution sol = solveExact(sc, g);
        if (sol.status != "FEASIBLE") continue;
        factibles++;
        double c = 0;
        CHECK(!sol.routes.empty() && judge(sc, g, sol.routes[0], c));
        CHECK(std::fabs(c - sol.cost) < 1e-6);
        CHECK(sol.cost >= bruteTsp(sc) - 1e-6);
    }
    CHECK(factibles > 0);

    if (fails == 0) std::cout << "OK: todas las pruebas del solver exacto pasaron\n";
    return fails == 0 ? 0 : 1;
}
