#pragma once

// Juez independiente: valida una ruta sin usar el solver.
// Complejidad temporal: lineal en el largo L de la ruta mas el costo de validar cada tramo
//   contra los obstaculos, O(L * (W + k)), con k = clientes.
// Complejidad espacial: O(L).
// Aplicabilidad: rutas de un solo dron desde la base; verifica visibilidad de cada tramo,
//   autonomia entre recargas y visita de todos los clientes.
#include <string>

#include "scenario.hpp"
#include "visibility.hpp"

struct JudgeResult {
    bool ok = false;
    double cost = 0;
    std::string reason;  // sale ok o sino el motivo del rechazo
};

// verifica una solucion, independientemente del resto del codigo.
JudgeResult judgeSolution(const Scenario& sc, const VisibilityGraph& g, const Solution& sol);