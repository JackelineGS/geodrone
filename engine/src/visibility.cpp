#include "visibility.hpp"

#include <algorithm>

std::string nodeLabel(const Node& n) {
    switch (n.type) {
        case NodeType::BASE: return "B";
        case NodeType::CLIENT: return "C" + std::to_string(n.id);
        case NodeType::STATION: return "S" + std::to_string(n.id);
        case NodeType::WAYPOINT: return "W" + std::to_string(n.id) + "." + std::to_string(n.vertex);
    }
    return "?";
}

int VisibilityGraph::edgeCount() const {
    size_t total = 0;
    for (const auto& a : adj) total += a.size();
    return (int)(total / 2);
}

int VisibilityGraph::indexOf(const std::string& label) const {
    for (int i = 0; i < size(); i++)
        if (nodeLabel(nodes[i]) == label) return i;
    return -1;
}

bool VisibilityGraph::hasEdge(int u, int v) const {
    for (const Edge& e : adj[u])
        if (e.to == v) return true;
    return false;
}

namespace {

struct Box {
    long long x0, y0, x1, y1;
};

Box boxOf(const Polygon& poly) {
    Box b{poly.v[0].x, poly.v[0].y, poly.v[0].x, poly.v[0].y};
    for (const Point& p : poly.v) {
        b.x0 = std::min(b.x0, p.x);
        b.y0 = std::min(b.y0, p.y);
        b.x1 = std::max(b.x1, p.x);
        b.y1 = std::max(b.y1, p.y);
    }
    return b;
}

// La caja del segmento toca la del poligono (bordes incluidos).
bool overlap(const Box& b, const Point& a, const Point& c) {
    return std::max(a.x, c.x) >= b.x0 && std::min(a.x, c.x) <= b.x1 &&
           std::max(a.y, c.y) >= b.y0 && std::min(a.y, c.y) <= b.y1;
}

}  // namespace

VisibilityGraph buildVisibilityGraph(const Scenario& sc) {
    VisibilityGraph g;
    g.nodes.push_back({NodeType::BASE, sc.baseId, 0, sc.base});
    for (const Client& c : sc.clients) g.nodes.push_back({NodeType::CLIENT, c.id, 0, c.p});
    for (const Station& s : sc.stations) g.nodes.push_back({NodeType::STATION, s.id, 0, s.p});
    for (const Polygon& poly : sc.polygons)
        for (size_t i = 0; i < poly.v.size(); i++)
            g.nodes.push_back({NodeType::WAYPOINT, poly.id, (int)i + 1, poly.v[i]});

    std::vector<Box> boxes;
    for (const Polygon& poly : sc.polygons) boxes.push_back(boxOf(poly));

    const int n = g.size();
    g.adj.assign(n, std::vector<Edge>());
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            const Point& a = g.nodes[i].p;
            const Point& b = g.nodes[j].p;
            bool ok = true;
            for (size_t k = 0; k < sc.polygons.size() && ok; k++) {
                if (!overlap(boxes[k], a, b)) continue;  // poda: no puede tocar este poligono
                if (!segmentValid(sc.polygons[k], a, b)) ok = false;
            }
            if (!ok) continue;
            double w = dist(a, b);
            g.adj[i].push_back({j, w});
            g.adj[j].push_back({i, w});
        }
    }
    return g;
}
