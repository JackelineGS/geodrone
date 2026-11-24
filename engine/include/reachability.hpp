#pragma once

// Alcanzabilidad de clientes (seccion 3.4).
// Complejidad temporal: DFS sobre la red de recarga O(m^2) mas una consulta por cliente sobre
//   distancias minimas; el costo dominante es el calculo de caminos minimos en el grafo
//   (R Dijkstra: O(R (V + E) log V), R = m + 1; el DFS cuesta O(R^2) y los clientes O(k R)).
// Complejidad espacial: O(V + k + m).
// Aplicabilidad: un cliente es alcanzable si hay ida y vuelta directa desde la base, o si llega
//   a una red de recarga conectada a la base (radio A / (2 kappa)).
#include <string>
#include <vector>

#include "scenario.hpp"
#include "visibility.hpp"

struct ClientReach {
    int id = 0;
    double dBase = 0;      // distancia minima base -> cliente (infinito si esta encerrado)
    bool direct = false;   // ida y vuelta desde la base, sin recargar
    bool reachable = false;  // alcanzable usando la red de recarga
    double dNear = 0;      // distancia al punto de recarga mas cercano de la red
    std::string via;       // "B" o "S#" (vacio si no hay)
};

struct ReachReport {
    double radius = 0;                   // A / (2 kappa)
    std::vector<std::string> network;    // base y estaciones conectadas a ella (B, S1, ...)
    std::vector<std::string> isolated;   // estaciones que no se pueden alcanzar desde la red
    std::vector<ClientReach> clients;
    bool allReachable = true;
};

// Alcanzabilidad (seccion 3.4): DFS sobre la red de recarga + condicion de ida y vuelta por cliente.
ReachReport analyzeReachability(const Scenario& sc, const VisibilityGraph& g);

// Informe en JSON (lo consume la API y el mapa).
std::string reachJson(const Scenario& sc, const ReachReport& rep);
