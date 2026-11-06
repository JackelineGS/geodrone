#include "param_search.hpp"

#include <algorithm>
#include <iomanip>
#include <limits>
#include <sstream>

#include "reachability.hpp"
#include "shortest_path.hpp"

namespace {
const double INF = std::numeric_limits<double>::infinity();
}

bool missionFeasible(const Scenario& sc, const VisibilityGraph& g, double A) {
    Scenario tmp = sc;
    tmp.drone.autonomy = A;
    return analyzeReachability(tmp, g).allReachable;
}

MinAutonomy minAutonomy(const Scenario& sc, const VisibilityGraph& g, double eps) {
    MinAutonomy r;
    const double kappa = sc.drone.kappa, kh = sc.drone.kh;

    // Cota superior: con este A todos los clientes son directos (ida y vuelta desde la base)
    ShortestPaths sp = dijkstra(g, 0);
    double hi = 0;
    for (size_t i = 0; i < sc.clients.size(); i++) {
        double d = sp.dist[1 + i];
        if (d == INF) return r;  // cliente encerrado: ninguna autonomia sirve
        hi = std::max(hi, 2 * kappa * d + kh * (double)sc.clients[i].s);
    }
    r.feasible = true;
    if (hi <= 0) return r;  // sin clientes o todos sobre la base

    double lo = 0;
    while (hi - lo > eps && r.iterations < 100) {
        double mid = (lo + hi) / 2;
        r.iterations++;
        if (missionFeasible(sc, g, mid))
            hi = mid;
        else
            lo = mid;
    }
    r.value = hi;
    return r;
}

std::string minAutonomyJson(const Scenario& sc, const MinAutonomy& r) {
    std::ostringstream o;
    o << std::fixed << std::setprecision(3);
    o << "{\"feasible\":" << (r.feasible ? "true" : "false");
    if (r.feasible) {
        double cur = sc.drone.autonomy;
        o << ",\"minAutonomy\":" << r.value << ",\"iterations\":" << r.iterations << ",\"current\":" << cur
          << ",\"margin\":" << (cur - r.value) << ",\"currentFeasible\":" << (cur >= r.value ? "true" : "false");
    }
    o << "}";
    return o.str();
}
