#pragma once
#include <string>
#include <vector>

#include "shortest_path.hpp"
#include "visibility.hpp"

// Agrega a `out` los nodos del camino minimo a -> b: las esquinas (W) y el destino.
// Si el camino pasa por un cliente, estacion o base que esta justo sobre un vertice de edificio
// (mismo punto que un W, unidos por una arista de longitud 0), se escribe la esquina W de ese punto.
// Omitirla haria que el tramo escrito cortara el edificio y el juez lo rechazaria.
// Complejidad: O(|camino|) y O(V) por cada nodo intermedio que no es esquina (raro).
inline void appendPathLabels(const VisibilityGraph& g, const ShortestPaths& sp, int a, int b,
                             std::vector<std::string>& out) {
    std::vector<int> p = reconstructPath(sp, a, b);
    for (size_t t = 1; t < p.size(); t++) {
        const Node& nd = g.nodes[p[t]];
        if (t + 1 == p.size() || nd.type == NodeType::WAYPOINT) {
            out.push_back(nodeLabel(nd));
            continue;
        }
        for (int w = 0; w < g.size(); w++) {
            if (g.nodes[w].type != NodeType::WAYPOINT || !(g.nodes[w].p == nd.p)) continue;
            std::string lab = nodeLabel(g.nodes[w]);
            if (out.empty() || out.back() != lab) out.push_back(lab);
            break;
        }
    }
}
