#pragma once

struct Point {
    long long x, y;
};

// Producto cruz de (b - a) x (c - a).
// > 0: giro antihorario (c a la izquierda de a->b)
// < 0: giro horario (c a la derecha)
// = 0: colineales
long long cross(const Point& a, const Point& b, const Point& c);
