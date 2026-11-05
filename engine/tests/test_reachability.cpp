#include <cmath>
#include <iostream>
#include <random>
#include <sstream>

#include "reachability.hpp"
#include "solver_exact.hpp"

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

static ReachReport analyze(const std::string& text) {
    Scenario sc = load(text);
    return analyzeReachability(sc, buildVisibilityGraph(sc));
}

// Un cliente a distancia 10 de la base (0,0)
static std::string unCliente(double A, double kh, int s) {
    std::ostringstream o;
    o << "SEED 1\nCATEGORY Small\n1 0 0\nA " << A << " KAPPA 1 V 1 Q 10 TAU 0 KH " << kh
      << "\nBASE 0 0\nCLIENT 1 10 0 1 1 0 1000 " << s << " 1\n";
    return o.str();
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
    // --- Ejemplo de la seccion 3.2 ---
    {
        Scenario sc = load(EJEMPLO);
        ReachReport r = analyzeReachability(sc, buildVisibilityGraph(sc));
        CHECK(std::fabs(r.radius - 10.0) < 1e-9);
        CHECK((r.network == std::vector<std::string>{"B", "S1"}));
        CHECK(r.isolated.empty() && r.allReachable && r.clients.size() == 2);
        const ClientReach& c1 = r.clients[0];
        const ClientReach& c2 = r.clients[1];
        CHECK(c1.id == 1 && !c1.direct && c1.reachable && c1.via == "S1");  // 2*11.21 > 20, pero via S1 si
        CHECK(std::fabs(c1.dBase - 11.211) < 1e-3);
        CHECK(std::fabs(c1.dNear - std::sqrt(20.0)) < 1e-9);
        CHECK(c2.direct && c2.reachable && c2.via == "B");
        std::string js = reachJson(sc, r);
        CHECK(js.find("\"allReachable\":true") != std::string::npos);
        CHECK(js.find("\"via\":\"S1\"") != std::string::npos);
    }

    // --- Sin estacion: C1 queda fuera de alcance (PT11) ---
    {
        std::string t = EJEMPLO;
        t.replace(t.find("2 1 1"), 5, "2 0 1");
        t.erase(t.find("STATION 1 9 0\n"), 14);
        ReachReport r = analyze(t);
        CHECK((r.network == std::vector<std::string>{"B"}));
        CHECK(!r.allReachable && !r.clients[0].reachable && r.clients[1].reachable);
        CHECK(std::fabs(r.clients[0].dNear - r.clients[0].dBase) < 1e-9);
    }

    // --- Estacion demasiado lejos de la red: aislada y sin utilidad ---
    {
        std::string t = EJEMPLO;
        t.replace(t.find("STATION 1 9 0"), 13, "STATION 1 40 40");
        ReachReport r = analyze(t);
        CHECK((r.network == std::vector<std::string>{"B"}));
        CHECK((r.isolated == std::vector<std::string>{"S1"}));
        CHECK(!r.clients[0].reachable);
    }

    // --- Casos limite: A igual a la distancia exacta ---
    CHECK(analyze(unCliente(20, 0, 0)).clients[0].direct);    // 2*10 = 20
    CHECK(!analyze(unCliente(19, 0, 0)).clients[0].direct);
    CHECK(analyze(unCliente(26, 0.2, 30)).clients[0].direct);   // 20 + 0.2*30 = 26
    CHECK(!analyze(unCliente(25, 0.2, 30)).clients[0].direct);

    // --- Coherencia con el solver: todos alcanzables <=> el solver halla ruta ---
    std::mt19937_64 rng(20261);
    const double autonomias[3] = {25, 35, 50};
    int factibles = 0, infactibles = 0;
    for (int caso = 0; caso < 40; caso++) {
        Scenario sc = load(randomScenario(rng, 2 + caso % 4, caso % 3, autonomias[caso % 3]));
        VisibilityGraph g = buildVisibilityGraph(sc);
        ReachReport r = analyzeReachability(sc, g);
        Solution sol = solveExact(sc, g);
        CHECK(r.allReachable == (sol.status == "FEASIBLE"));
        (r.allReachable ? factibles : infactibles)++;
    }
    std::cout << "(aleatorios: " << factibles << " factibles, " << infactibles << " infactibles)\n";

    if (fails == 0) std::cout << "OK: todas las pruebas de alcanzabilidad pasaron\n";
    return fails == 0 ? 0 : 1;
}
