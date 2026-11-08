#include <cmath>
#include <iostream>
#include <sstream>

#include "reachability.hpp"
#include "station_planner.hpp"

static int fails = 0;
#define CHECK(cond)                                                     \
    do {                                                                \
        if (!(cond)) {                                                  \
            std::cout << "FALLO linea " << __LINE__ << ": " #cond "\n"; \
            fails++;                                                    \
        }                                                               \
    } while (0)

static Scenario load(const std::string& text) {
    Scenario sc;
    std::string err;
    std::istringstream in(text);
    bool ok = readScenario(in, sc, err);
    CHECK(ok);
    return sc;
}

// Escenario sin clientes ni estaciones: base en (bx,by), autonomia A, kh = 0.2 y una lista de poligonos
static std::string base(int bx, int by, double A, const std::string& polys, int np) {
    std::ostringstream o;
    o << "SEED 1\nCATEGORY Large\n0 0 " << np << "\nA " << A << " KAPPA 1 V 10 Q 10 TAU 60 KH 0.2\nBASE " << bx
      << " " << by << "\n"
      << polys;
    return o.str();
}

static const std::vector<Point> CUADRADO = {{0, 0}, {1000, 0}, {1000, 1000}, {0, 1000}};

// Contraste independiente: se agrega un cliente por cada punto de la malla (con s = 30 s) y se usa
// el analisis de alcanzabilidad, que calcula todo con Dijkstra sobre el grafo de visibilidad completo.
static void contrastar(const Scenario& sc, const PlanResult& r) {
    Scenario t = withStations(sc, r);
    int id = 1;
    for (const Point& p : r.grid) {
        Client c;
        c.id = id++;
        c.p = p;
        c.s = 30;
        t.clients.push_back(c);
    }
    VisibilityGraph g = buildVisibilityGraph(t);
    ReachReport rep = analyzeReachability(t, g);
    CHECK(rep.isolated.empty());  // todas las estaciones nuevas quedan conectadas a la base
    int coberturaIndependiente = 0;
    for (size_t i = 0; i < r.grid.size(); i++) {
        CHECK(rep.clients[i].reachable == (r.isCovered[i] != 0));
        coberturaIndependiente += rep.clients[i].reachable;
    }
    CHECK(coberturaIndependiente == r.covered);
}

int main() {
    PlannerOptions opt;
    opt.step = 100;

    // --- Sin obstaculos: cobertura total y estaciones conectadas ---
    {
        Scenario sc = load(base(500, 500, 700, "", 0));  // radio = (700 - 6) / 2 = 347
        PlanResult r = planStations(sc, CUADRADO, opt);
        CHECK(std::fabs(r.radius - 347.0) < 1e-9);
        CHECK(r.grid.size() == 121);  // malla 11 x 11
        CHECK(r.uncovered.empty() && r.covered == 121);
        CHECK(!r.stations.empty());
        contrastar(sc, r);
        // sin obstaculos la distancia es la euclidiana: cada punto esta a <= radio de la base o de una estacion
        for (const Point& p : r.grid) {
            double d = dist(p, sc.base);
            for (const PlannedStation& s : r.stations) d = std::min(d, dist(p, s.p));
            CHECK(d <= r.radius + 1e-9);
        }
    }

    // --- Con una autonomia grande, la base sola cubre todo: no hacen falta estaciones ---
    {
        Scenario sc = load(base(500, 500, 2000, "", 0));
        PlanResult r = planStations(sc, CUADRADO, opt);
        CHECK(r.stations.empty() && r.uncovered.empty());
    }

    // --- Con un obstaculo grande en medio: los rodeos reducen la cobertura ---
    const std::string muro = "POLYGON 1 4\n300 400\n700 400\n700 600\n300 600\n";
    {
        Scenario sc = load(base(100, 500, 700, muro, 1));
        PlanResult r = planStations(sc, CUADRADO, opt);
        CHECK(r.grid.size() < 121);  // los puntos dentro del obstaculo no cuentan
        CHECK(r.uncovered.empty());
        contrastar(sc, r);

        // mas estaciones que sin obstaculo (la base esta pegada a un lado)
        Scenario libre = load(base(100, 500, 700, "", 0));
        PlanResult rl = planStations(libre, CUADRADO, opt);
        CHECK(r.stations.size() >= rl.stations.size());
    }

    // --- Estaciones existentes: se aprovechan ---
    {
        Scenario sc = load(
            "SEED 1\nCATEGORY Large\n0 1 0\nA 700 KAPPA 1 V 10 Q 10 TAU 60 KH 0.2\nBASE 100 100\nSTATION 7 800 800\n");
        PlanResult r = planStations(sc, CUADRADO, opt);
        for (const PlannedStation& s : r.stations) CHECK(s.id > 7);  // ids nuevos no repiten
        Scenario sinEst = load(base(100, 100, 700, "", 0));
        PlanResult r0 = planStations(sinEst, CUADRADO, opt);
        CHECK(r.stations.size() <= r0.stations.size());
        contrastar(sc, r);
    }

    // --- Autonomia demasiado pequena: radio <= 0, nada se cubre y no se cuelga ---
    {
        Scenario sc = load(base(500, 500, 5, "", 0));
        PlanResult r = planStations(sc, CUADRADO, opt);
        CHECK(r.radius <= 0 && r.stations.empty() && r.covered == 0 && r.uncovered.size() == r.grid.size());
    }

    // --- Determinismo ---
    {
        Scenario sc = load(base(100, 500, 700, muro, 1));
        PlanResult a = planStations(sc, CUADRADO, opt), b = planStations(sc, CUADRADO, opt);
        CHECK(a.stations.size() == b.stations.size());
        for (size_t i = 0; i < a.stations.size(); i++) CHECK(a.stations[i].p == b.stations[i].p);
    }

    // --- writeScenario -> readScenario: ida y vuelta, y el resultado se puede resolver ---
    {
        Scenario sc = load(base(100, 500, 700, muro, 1));
        PlanResult r = planStations(sc, CUADRADO, opt);
        Scenario con = withStations(sc, r);
        std::ostringstream out;
        writeScenario(out, con);
        Scenario back = load(out.str());
        CHECK(back.stations.size() == con.stations.size());
        CHECK(back.polygons.size() == 1 && back.polygons[0].v.size() == 4);
        CHECK(back.base == con.base && back.drone.autonomy == 700 && back.drone.kh == 0.2);
        std::string err;
        CHECK(validateScenario(back, err));
        for (size_t i = 0; i < con.stations.size(); i++) CHECK(back.stations[i].p == con.stations[i].p);
    }

    if (fails == 0) std::cout << "OK: todas las pruebas del planificador de estaciones pasaron\n";
    return fails == 0 ? 0 : 1;
}
