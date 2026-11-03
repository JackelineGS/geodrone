#include "geometry.hpp"

#include <algorithm>
#include <cmath>

long long cross(const Point& a, const Point& b, const Point& c) {
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

int orient(const Point& a, const Point& b, const Point& c) {
    long long c0 = cross(a, b, c);
    return (c0 > 0) - (c0 < 0);
}

bool onSegment(const Point& a, const Point& b, const Point& p) {
    if (cross(a, b, p) != 0) return false;
    return std::min(a.x, b.x) <= p.x && p.x <= std::max(a.x, b.x) &&
           std::min(a.y, b.y) <= p.y && p.y <= std::max(a.y, b.y);
}

bool segmentsIntersect(const Segment& s, const Segment& t) {
    int o1 = orient(s.a, s.b, t.a), o2 = orient(s.a, s.b, t.b);
    int o3 = orient(t.a, t.b, s.a), o4 = orient(t.a, t.b, s.b);
    if (o1 * o2 < 0 && o3 * o4 < 0) return true;  // cruce propio
    // casos degenerados: un extremo toca al otro segmento
    if (onSegment(s.a, s.b, t.a) || onSegment(s.a, s.b, t.b)) return true;
    if (onSegment(t.a, t.b, s.a) || onSegment(t.a, t.b, s.b)) return true;
    return false;
}

bool properCross(const Segment& s, const Segment& t) {
    int o1 = orient(s.a, s.b, t.a), o2 = orient(s.a, s.b, t.b);
    int o3 = orient(t.a, t.b, s.a), o4 = orient(t.a, t.b, s.b);
    return o1 * o2 < 0 && o3 * o4 < 0;
}

Location locatePoint(const Polygon& poly, const Point& p) {
    const auto& v = poly.v;
    const size_t n = v.size();
    // 1) frontera: sobre una arista o vértice
    for (size_t i = 0; i < n; i++)
        if (onSegment(v[i], v[(i + 1) % n], p)) return Location::BOUNDARY;
    // 2) rayo hacia +x con regla semiabierta (sin doble conteo en vértices)
    bool inside = false;
    for (size_t i = 0; i < n; i++) {
        const Point& a = v[i];
        const Point& b = v[(i + 1) % n];
        if ((a.y > p.y) != (b.y > p.y)) {
            long long c = cross(a, b, p);
            if (b.y > a.y ? c > 0 : c < 0) inside = !inside;
        }
    }
    return inside ? Location::INTERIOR : Location::EXTERIOR;
}

bool segmentValid(const Polygon& poly, const Point& a, const Point& b) {
    if (a == b) return locatePoint(poly, a) != Location::INTERIOR;

    const auto& v = poly.v;
    const size_t n = v.size();
    const Segment s{a, b};

    // Un cruce propio con una arista siempre entra al interior
    for (size_t i = 0; i < n; i++)
        if (properCross(s, Segment{v[i], v[(i + 1) % n]})) return false;

    // Sin cruces propios, el vuelo solo toca la frontera en sus extremos o en vértices
    // que caen sobre él. Se parte en tramos y se revisa el punto medio de cada uno.
    std::vector<Point> cuts{a, b};
    for (const Point& p : v)
        if (onSegment(a, b, p)) cuts.push_back(p);

    auto t = [&](const Point& p) { return (p.x - a.x) * (b.x - a.x) + (p.y - a.y) * (b.y - a.y); };
    std::sort(cuts.begin(), cuts.end(), [&](const Point& p, const Point& q) { return t(p) < t(q); });

    // Se duplican las coordenadas para que el punto medio siga siendo entero
    Polygon big;
    for (const Point& p : v) big.v.push_back({2 * p.x, 2 * p.y});

    for (size_t k = 0; k + 1 < cuts.size(); k++) {
        if (cuts[k] == cuts[k + 1]) continue;
        Point mid{cuts[k].x + cuts[k + 1].x, cuts[k].y + cuts[k + 1].y};
        if (locatePoint(big, mid) == Location::INTERIOR) return false;
    }
    return true;
}

double dist(const Point& a, const Point& b) {
    return std::hypot(double(a.x - b.x), double(a.y - b.y));
}
