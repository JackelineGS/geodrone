#include "reachability.hpp"

#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>

#include "shortest_path.hpp"

namespace {

const double INF = std::numeric_limits<double>::infinity();
const double EPS = 1e-9;

void num(std::ostream& o, double v) {
    if (std::isfinite(v))
        o << std::fixed << std::setprecision(3) << v;
    else
        o << "null";
}

void strList(std::ostream& o, const std::vector<std::string>& v) {
    o << "[";
    for (size_t i = 0; i < v.size(); i++) o << (i ? "," : "") << "\"" << v[i] << "\"";
    o << "]";
}

}  // namespace

ReachReport analyzeReachability(const Scenario& sc, const VisibilityGraph& g) {
    const int k = (int)sc.clients.size();
    const int m = (int)sc.stations.size();
    const int R = 1 + m;  // puntos de recarga: 0 = base, 1..m = estaciones
    // Orden de nodos en el grafo: base, clientes, estaciones
    auto rn = [&](int r) { return r == 0 ? 0 : k + r; };
    auto rlabel = [&](int r) { return r == 0 ? std::string("B") : "S" + std::to_string(sc.stations[r - 1].id); };

    const double kappa = sc.drone.kappa, kh = sc.drone.kh, A = sc.drone.autonomy;

    ReachReport rep;
    rep.radius = kappa > 0 ? A / (2 * kappa) : INF;

    std::vector<ShortestPaths> sp;
    for (int r = 0; r < R; r++) sp.push_back(dijkstra(g, rn(r)));

    // DFS desde la base: dos puntos de recarga se conectan si el vuelo cabe en una bateria
    std::vector<char> in(R, 0);
    std::vector<int> stack{0};
    in[0] = 1;
    while (!stack.empty()) {
        int r = stack.back();
        stack.pop_back();
        for (int r2 = 0; r2 < R; r2++) {
            if (in[r2]) continue;
            double d = sp[r].dist[rn(r2)];
            if (d == INF || kappa * d > A + EPS) continue;
            in[r2] = 1;
            stack.push_back(r2);
        }
    }
    for (int r = 0; r < R; r++) (in[r] ? rep.network : rep.isolated).push_back(rlabel(r));

    for (int c = 0; c < k; c++) {
        const int node = 1 + c;
        const Client& cl = sc.clients[c];
        const double serv = kh * (double)cl.s;

        ClientReach cr;
        cr.id = cl.id;
        cr.dBase = sp[0].dist[node];
        cr.direct = cr.dBase != INF && 2 * kappa * cr.dBase + serv <= A + EPS;

        cr.dNear = INF;
        int via = -1;
        for (int r = 0; r < R; r++) {
            if (!in[r]) continue;
            double d = sp[r].dist[node];
            if (d < cr.dNear) {
                cr.dNear = d;
                via = r;
            }
        }
        cr.reachable = via >= 0 && cr.dNear != INF && 2 * kappa * cr.dNear + serv <= A + EPS;
        if (via >= 0 && cr.dNear != INF) cr.via = rlabel(via);
        if (!cr.reachable) rep.allReachable = false;
        rep.clients.push_back(cr);
    }
    return rep;
}

std::string reachJson(const Scenario& sc, const ReachReport& rep) {
    std::ostringstream o;
    o << "{\"radius\":";
    num(o, rep.radius);
    o << ",\"base\":{\"x\":" << sc.base.x << ",\"y\":" << sc.base.y << "}";
    o << ",\"network\":";
    strList(o, rep.network);
    o << ",\"isolated\":";
    strList(o, rep.isolated);
    o << ",\"allReachable\":" << (rep.allReachable ? "true" : "false") << ",\"clients\":[";
    for (size_t i = 0; i < rep.clients.size(); i++) {
        const ClientReach& c = rep.clients[i];
        o << (i ? "," : "") << "{\"id\":" << c.id << ",\"direct\":" << (c.direct ? "true" : "false")
          << ",\"reachable\":" << (c.reachable ? "true" : "false") << ",\"dBase\":";
        num(o, c.dBase);
        o << ",\"dNear\":";
        num(o, c.dNear);
        o << ",\"via\":";
        if (c.via.empty())
            o << "null";
        else
            o << "\"" << c.via << "\"";
        o << "}";
    }
    o << "]}";
    return o.str();
}
