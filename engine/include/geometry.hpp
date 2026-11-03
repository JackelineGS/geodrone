#pragma once
#include <vector>

struct Point {
    long long x, y;
};
inline bool operator==(const Point& p, const Point& q) { return p.x == q.x && p.y == q.y; }

struct Segment {
    Point a, b;
};

struct Polygon {
    int id = 0;
    std::vector<Point> v;  // vértices en sentido antihorario
};

enum class Location { EXTERIOR, BOUNDARY, INTERIOR };

// Producto cruz (b - a) x (c - a): >0 antihorario, <0 horario, 0 colineales
long long cross(const Point& a, const Point& b, const Point& c);

// Signo de cross: +1, -1 o 0
int orient(const Point& a, const Point& b, const Point& c);

// p está sobre el segmento cerrado ab (extremos incluidos)
bool onSegment(const Point& a, const Point& b, const Point& p);

// Los segmentos cerrados comparten al menos un punto (cruce, T, extremo, solape)
bool segmentsIntersect(const Segment& s, const Segment& t);

// Cruce propio: se cortan en un punto interior a ambos, sin tocar extremos
bool properCross(const Segment& s, const Segment& t);

// Ubicación de un punto respecto a un polígono simple (convexo o cóncavo)
Location locatePoint(const Polygon& poly, const Point& p);

// Regla de validez: el vuelo ab es válido si NO entra al interior del polígono.
// Rozar un vértice o recorrer una arista es válido.
bool segmentValid(const Polygon& poly, const Point& a, const Point& b);

double dist(const Point& a, const Point& b);
