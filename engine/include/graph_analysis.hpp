#pragma once
#include <string>
#include <vector>

#include "scenario.hpp"
#include "visibility.hpp"

struct MstEdge {
    std::string a, b;  // etiquetas: B, S1, S2...
    double d = 0;      // distancia minima entre ambos (camino minimo, esquivando edificios)
};

struct NetworkAnalysis {
    int nodes = 0;                          // base + estaciones
    int edges = 0;                          // aristas de la red de recarga (kappa * d <= A)
    int components = 0;                     // componentes conexas (DFS)
    std::vector<std::string> articulation;  // puntos de articulacion (RF11)
    std::vector<MstEdge> mst;               // bosque de expansion minima (RF12)
    double mstCost = 0;                     // costo de infraestructura = suma de las aristas del MST
    bool connected = true;                  // true si toda la red queda en una sola componente
};

// Red de recarga: nodos = base + estaciones; arista i-j si kappa * d(i,j) <= A (d = camino minimo).
// Articulaciones por DFS con disc/low: O(V + E). MST por Kruskal + union-busqueda: O(E log E).
NetworkAnalysis analyzeNetwork(const Scenario& sc, const VisibilityGraph& g);

// Salida en JSON (igual que reachJson / minAutonomyJson).
std::string networkJson(const NetworkAnalysis& r);
