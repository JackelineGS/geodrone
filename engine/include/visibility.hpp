#pragma once

// Grafo de visibilidad entre base, clientes, estaciones y vertices de poligonos.
// Sea V = 1 + k + m + W nodos (W = vertices de poligonos) y E aristas.
// Complejidad temporal: construccion O(V^2 * W) si cada par se prueba contra todas las aristas
//   de obstaculos (O(V^3) en el peor caso); cada poligono tiene una caja envolvente precalculada.
// Complejidad espacial: O(V + E), con E <= V^2.
// Aplicabilidad: obstaculos poligonales simples; una arista es valida si el vuelo recto no entra
//   al interior de ningun poligono (rozar un vertice o recorrer una arista es valido).
#include <string>
#include <vector>

#include "scenario.hpp"

enum class NodeType { BASE, CLIENT, STATION, WAYPOINT };

struct Node {
    NodeType type = NodeType::BASE;
    int id = 0;      // id del cliente/estacion, o del poligono si es WAYPOINT
    int vertex = 0;  // vertice 1..nv dentro del poligono (solo WAYPOINT)
    Point p{0, 0};
};

std::string nodeLabel(const Node& n);  // B, C2, S1, W1.1

struct Edge {
    int to;
    double w;
};

struct VisibilityGraph {
    std::vector<Node> nodes;
    std::vector<std::vector<Edge>> adj;  // no dirigido: cada arista aparece en ambos sentidos

    int size() const { return (int)nodes.size(); }
    int edgeCount() const;
    int indexOf(const std::string& label) const;  // -1 si no existe
    bool hasEdge(int u, int v) const;
};

// Nodos: base, clientes, estaciones y vertices de poligonos.
// Arista u-v si el vuelo recto no entra al interior de ningun obstaculo; peso = distancia.
VisibilityGraph buildVisibilityGraph(const Scenario& sc);
