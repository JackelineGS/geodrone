#include "judge.hpp"

#include <cmath>
#include <set>

JudgeResult judgeSolution(const Scenario& sc, const VisibilityGraph& g, const Solution& sol) {
    JudgeResult r;
    if (sol.status != "FEASIBLE" || sol.routes.empty()) {
        r.reason = "no hay ruta que verificar";
        return r;
    }
    const std::vector<std::string>& route = sol.routes[0];
    if (route.size() < 2 || route.front() != "B" || route.back() != "B") {
        r.reason = "la ruta no empieza y termina en B";
        return r;
    }

    double battery = sc.drone.autonomy;  // el dron sale de la base con bateria a full
    std::set<int> visited;
    Point prev = sc.base;

    for (size_t i = 1; i < route.size(); i++) {
        int idx = g.indexOf(route[i]);
        if (idx < 0) {
            r.reason = "etiqueta desconocida: " + route[i];
            return r;
        }
        const Node& nd = g.nodes[idx];

        // 1. el tramo recto no puede entrar a ningun edificio
        for (const Polygon& poly : sc.polygons) {
            if (!segmentValid(poly, prev, nd.p)) {
                r.reason = "el tramo hacia " + route[i] + " entra a un edificio";
                return r;
            }
        }

        // 2. Gasto de bateria: vuelo + tiempo de servicio si es cliente
        double d = dist(prev, nd.p);
        r.cost += d;
        battery -= sc.drone.kappa * d;
        if (nd.type == NodeType::CLIENT) {
            visited.insert(nd.id);
            for (const Client& c : sc.clients)
                if (c.id == nd.id) battery -= sc.drone.kh * (double)c.s;
        }
        if (battery < -1e-9) {
            r.reason = "bateria agotada antes de llegar a " + route[i];
            return r;
        }

        // 3) base o estacion, se recarga completamente
        if (nd.type == NodeType::BASE || nd.type == NodeType::STATION) battery = sc.drone.autonomy;
        prev = nd.p;
    }

    for (const Client& c : sc.clients) {
        if (c.mandatory && !visited.count(c.id)) {
            r.reason = "falta el cliente C" + std::to_string(c.id);
            return r;
        }
    }
    if (std::fabs(r.cost - sol.cost) > 1e-6) {
        r.reason = "el costo reportado no coincide con el recalculado";
        return r;
    }
    r.ok = true;
    r.reason = "ok";
    return r;
}