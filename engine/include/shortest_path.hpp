#pragma once
#include <string>
#include <vector>

#include "visibility.hpp"

struct ShortestPaths {
    std::vector<double> dist;  // dist[v]: distancia minima desde el origen (infinito si no hay camino)
    std::vector<int> parent;   // parent[v]: nodo previo en el camino (-1 si no hay)
};

// Dijkstra con cola de prioridad. Pesos >= 0. O((V+E) log V).
ShortestPaths dijkstra(const VisibilityGraph& g, int s);

// Camino s -> t como lista de indices de nodos. Vacio si t no es alcanzable.
std::vector<int> reconstructPath(const ShortestPaths& sp, int s, int t);

// Indices de nodos -> etiquetas (B, C2, S1, W1.1), formato de la salida.
std::vector<std::string> pathLabels(const VisibilityGraph& g, const std::vector<int>& path);

// Matriz completa de distancias minimas. O(V^3). Solo para validar Dijkstra y casos pequenos.
std::vector<std::vector<double>> floydWarshall(const VisibilityGraph& g);
