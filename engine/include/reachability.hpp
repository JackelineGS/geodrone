#pragma once
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
