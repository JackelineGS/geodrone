#include <iostream>

#include "geometry.hpp"

static int fails = 0;
#define CHECK(cond)                                                     \
    do {                                                                \
        if (!(cond)) {                                                  \
            std::cout << "FALLO linea " << __LINE__ << ": " #cond "\n"; \
            fails++;                                                    \
        }                                                               \
    } while (0)

int main() {
    // Cuadrado [4,8] x [2,6]
    Polygon sq{1, {{4, 2}, {8, 2}, {8, 6}, {4, 6}}};
    // Polígono en L (cóncavo)
    Polygon L{2, {{0, 0}, {6, 0}, {6, 2}, {2, 2}, {2, 6}, {0, 6}}};

    // PT01 orientación
    CHECK(cross({0, 0}, {4, 0}, {2, 3}) > 0);
    CHECK(cross({0, 0}, {4, 0}, {2, -3}) < 0);
    CHECK(cross({0, 0}, {4, 0}, {9, 0}) == 0);

    // PT02 intersección de segmentos
    CHECK(segmentsIntersect({{0, 0}, {4, 4}}, {{0, 4}, {4, 0}}));      // cruce propio
    CHECK(segmentsIntersect({{0, 0}, {4, 0}}, {{2, 0}, {2, 5}}));      // en T
    CHECK(segmentsIntersect({{0, 0}, {2, 2}}, {{2, 2}, {5, 0}}));      // extremo con extremo
    CHECK(segmentsIntersect({{0, 0}, {4, 0}}, {{2, 0}, {6, 0}}));      // colineales solapados
    CHECK(!segmentsIntersect({{0, 0}, {1, 0}}, {{3, 0}, {5, 0}}));     // colineales disjuntos
    CHECK(!properCross({{0, 0}, {2, 2}}, {{2, 2}, {5, 0}}));           // tocarse no es cruce propio

    // PT03 punto en polígono
    CHECK(locatePoint(sq, {6, 4}) == Location::INTERIOR);
    CHECK(locatePoint(sq, {0, 0}) == Location::EXTERIOR);
    CHECK(locatePoint(sq, {4, 2}) == Location::BOUNDARY);  // vértice
    CHECK(locatePoint(sq, {6, 2}) == Location::BOUNDARY);  // borde
    CHECK(locatePoint(sq, {0, 2}) == Location::EXTERIOR);  // alineado con un vértice
    CHECK(locatePoint(sq, {0, 6}) == Location::EXTERIOR);
    CHECK(locatePoint(L, {4, 4}) == Location::EXTERIOR);   // dentro de la muesca de la L
    CHECK(locatePoint(L, {1, 5}) == Location::INTERIOR);

    // PT04 rozar un vértice es válido
    CHECK(segmentValid(sq, {6, 0}, {10, 4}));
    // PT05 volar a lo largo de una arista es válido
    CHECK(segmentValid(sq, {4, 2}, {8, 2}));
    CHECK(segmentValid(sq, {0, 2}, {12, 2}));

    // PT06 entrar al interior es inválido
    CHECK(!segmentValid(sq, {0, 4}, {12, 4}));    // cruce propio
    CHECK(!segmentValid(sq, {4, 2}, {8, 6}));     // diagonal vértice a vértice
    CHECK(!segmentValid(sq, {2, 0}, {10, 8}));    // colineal con la diagonal, pasa por 2 vértices
    CHECK(!segmentValid(sq, {5, 3}, {7, 5}));     // totalmente dentro
    CHECK(!segmentValid(L, {0, 0}, {6, 2}));      // atraviesa el interior cóncavo
    CHECK(!segmentValid(L, {0, 6}, {6, 0}));      // entra por la parte superior de la L
    CHECK(segmentValid(L, {6, 2}, {2, 6}));       // cruza la muesca (exterior) entre dos vértices

    if (fails == 0) std::cout << "OK: todas las pruebas de geometria pasaron\n";
    return fails == 0 ? 0 : 1;
}
