#include <cmath>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "param_search.hpp"
#include "reachability.hpp"
#include "scenario.hpp"
#include "visibility.hpp"

struct Cfg { int kMin, kMax, mMin, mMax, pMin, pMax; long long X; double A; };

// Rangos por categoría, lado del mapa y autonomía
static Cfg cfgOf(const std::string& c, bool& ok) {
    ok = true;
    if (c == "Small")   return {5, 12, 1, 3, 1, 5, 1000, 1500};
    if (c == "Medium")  return {13, 100, 3, 10, 5, 30, 5000, 3000};
    if (c == "Large")   return {101, 300, 10, 50, 30, 100, 10000, 4000};
    if (c == "Extreme") return {301, 1000, 50, 100, 100, 300, 20000, 5000};
    ok = false;
    return {};
}

// Entero en [lo, hi]. Uso % y no uniform_int_distribution para que la misma semilla de archivos iguales con cualquier compilador.
static long long rnd(std::mt19937_64& g, long long lo, long long hi) {
    return lo + (long long)(g() % (unsigned long long)(hi - lo + 1));
}

// Rectángulo con vértices en sentido antihorario
static Polygon rect(int id, long long x, long long y, long long w, long long h) {
    Polygon p;
    p.id = id;
    p.v = {{x, y}, {x + w, y}, {x + w, y + h}, {x, y + h}};
    return p;
}

// ¿Los interiores de a y b se solapan? (ojo, solo para rectangulos)
static bool overlap(const Polygon& a, const Polygon& b, long long gap) {
    return a.v[0].x < b.v[2].x + gap && b.v[0].x < a.v[2].x + gap &&
           a.v[0].y < b.v[2].y + gap && b.v[0].y < a.v[2].y + gap;
}

// Punto libre: fuera de todos los edificios (ni siquiera en el borde) y sin otro nodo encima
static bool freeSpot(const Scenario& sc, const Point& p) {
    for (const Polygon& poly : sc.polygons)
        if (locatePoint(poly, p) != Location::EXTERIOR) return false;
    if (p == sc.base) return false;
    for (const Client& c : sc.clients) if (c.p == p) return false;
    for (const Station& s : sc.stations) if (s.p == p) return false;
    return true;
}

static Point spot(const Scenario& sc, std::mt19937_64& g, long long X) {
    for (;;) {
        long long x = rnd(g, 0, X);
        long long y = rnd(g, 0, X);
        Point p{x, y};
        if (freeSpot(sc, p)) return p;
    }
}

static Scenario randomScenario(const std::string& cat, const Cfg& cfg, long long seed,
                               std::mt19937_64& g, bool wall) {
    Scenario sc;
    sc.seed = seed;
    sc.category = cat;
    sc.drone = {cfg.A, 1, 10, 1000, 60, 0.2};  // A, kappa, v, Q, tau, kh
    long long X = cfg.X;

    if (wall) {  // muro vertical de 5 bloques con huecos de 2 unidades
        long long slot = X / 5;
        for (int i = 0; i < 5; i++)
            sc.polygons.push_back(rect(i + 1, X / 2 - 10, i * slot + 1, 20, slot - 2));
    } else {
        int p = (int)rnd(g, cfg.pMin, cfg.pMax);
        for (int i = 0; i < p; i++) {
            for (int t = 0; t < 200; t++) {
                long long w = rnd(g, X / 50, X / 10);
                long long h = rnd(g, X / 50, X / 10);
                long long x = rnd(g, 0, X - w);
                long long y = rnd(g, 0, X - h);
                Polygon r = rect((int)sc.polygons.size() + 1, x, y, w, h);
                bool ok = true;
                for (const Polygon& q : sc.polygons)
                    if (overlap(q, r, 2)) ok = false;
                if (ok) { sc.polygons.push_back(r); break; }
            }
        }
    }

    sc.base = spot(sc, g, X);
    int k = (int)rnd(g, cfg.kMin, cfg.kMax);
    int m = (int)rnd(g, cfg.mMin, cfg.mMax);
    for (int i = 1; i <= k; i++) {
        Client c;
        c.id = i;
        c.p = spot(sc, g, X);
        c.prio = 1 + (int)rnd(g, 0, 2);
        c.mandatory = true;
        c.e = 0; c.l = 1000000; c.s = 30; c.demand = 1;
        sc.clients.push_back(c);
    }
    for (int i = 1; i <= m; i++) {
        Station s;
        s.id = i;
        s.p = spot(sc, g, X);
        sc.stations.push_back(s);
    }
    return sc;
}

static bool allReachable(const Scenario& sc) {
    VisibilityGraph vg = buildVisibilityGraph(sc);
    for (const ClientReach& c : analyzeReachability(sc, vg).clients)
        if (!c.reachable) return false;
    return true;
}

// repetir el sorteo con la misma semilla hasta que todos sean alcanzables
static Scenario makeReachable(const std::string& cat, const Cfg& cfg, long long seed,
                              std::mt19937_64& g, bool wall) {
    Scenario sc;
    for (int t = 0; t < 200; t++) {
        sc = randomScenario(cat, cfg, seed, g, wall);
        if (allReachable(sc)) return sc;
    }
    std::cerr << "Aviso: no se logro un caso totalmente alcanzable\n";
    return sc;
}

// caso colineal, clientes sobre vertices y sobre aristas de los edificios
static void alignClients(Scenario& sc) {
    size_t ci = 0;
    for (const Polygon& poly : sc.polygons) {
        if (ci < sc.clients.size()) sc.clients[ci++].p = poly.v[0];
        if (ci < sc.clients.size())
            sc.clients[ci++].p = Point{(poly.v[0].x + poly.v[1].x) / 2, poly.v[0].y};
    }
}

// Caso encerrado, un cliente nuevo rodeado por 4 rectángulos que se solapan en las esquinas
static void addEnclosedClient(Scenario& sc, long long X, std::mt19937_64& g) {
    long long d = X / 100, t = X / 100, R = d + t;
    for (int tries = 0; tries < 500; tries++) {
        long long cx = R + rnd(g, 0, X - 2 * R);
        long long cy = R + rnd(g, 0, X - 2 * R);
        Polygon box = rect(0, cx - R, cy - R, 2 * R, 2 * R);
        auto inBox = [&](const Point& q) {
            return q.x >= cx - R && q.x <= cx + R && q.y >= cy - R && q.y <= cy + R;
        };
        bool ok = !inBox(sc.base);
        for (const Polygon& p : sc.polygons) if (overlap(p, box, 0)) ok = false;
        for (const Client& c : sc.clients) if (inBox(c.p)) ok = false;
        for (const Station& s : sc.stations) if (inBox(s.p)) ok = false;
        if (!ok) continue;
        int id = (int)sc.polygons.size() + 1;
        sc.polygons.push_back(rect(id,     cx - R, cy + d, 2 * R, t));  // norte
        sc.polygons.push_back(rect(id + 1, cx - R, cy - R, 2 * R, t));  // sur
        sc.polygons.push_back(rect(id + 2, cx - R, cy - R, t, 2 * R));  // oeste
        sc.polygons.push_back(rect(id + 3, cx + d, cy - R, t, 2 * R));  // este
        Client c;
        c.id = (int)sc.clients.size() + 1;
        c.p = Point{cx, cy};
        c.prio = 1; c.mandatory = true;
        c.e = 0; c.l = 1000000; c.s = 30; c.demand = 1;
        sc.clients.push_back(c);
        return;
    }
    std::cerr << "Aviso: no se pudo colocar el cliente encerrado\n";
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Uso: main_generator <Small|Medium|Large|Extreme> <semilla> "
                     "<aleatorio|colineal|encerrado|limite|pasillo>\n";
        return 1;
    }
    std::string cat = argv[1], kind = argv[3];
    long long seed = std::stoll(argv[2]);
    bool ok;
    Cfg cfg = cfgOf(cat, ok);
    if (!ok) { std::cerr << "Categoria invalida\n"; return 1; }

    std::mt19937_64 g(seed);
    Scenario sc;
    if (kind == "aleatorio") {
        sc = makeReachable(cat, cfg, seed, g, false);
    } else if (kind == "pasillo") {
        sc = makeReachable(cat, cfg, seed, g, true);
    } else if (kind == "colineal") {
        sc = makeReachable(cat, cfg, seed, g, false);
        alignClients(sc);
    } else if (kind == "encerrado") {
        sc = makeReachable(cat, cfg, seed, g, false);
        addEnclosedClient(sc, cfg.X, g);
    } else if (kind == "limite") {
        sc = makeReachable(cat, cfg, seed, g, false);
        VisibilityGraph vg = buildVisibilityGraph(sc);
        MinAutonomy m = minAutonomy(sc, vg);
        if (m.feasible) sc.drone.autonomy = std::ceil(m.value * 1000) / 1000;
    } else {
        std::cerr << "Tipo invalido\n";
        return 1;
    }

    std::string err;
    if (!validateScenario(sc, err)) {
        std::cerr << "Escenario invalido: " << err << "\n";
        return 1;
    }
    writeScenario(std::cout, sc);
    return 0;
}