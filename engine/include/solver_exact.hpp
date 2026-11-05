#pragma once
#include "scenario.hpp"
#include "visibility.hpp"

// Solver exacto (branch and bound + DP de recargas) para una base y un dron.
// Visita todos los clientes. Si se agota el tiempo, devuelve la mejor ruta encontrada.
Solution solveExact(const Scenario& sc, const VisibilityGraph& g, double timeLimitMs = 900.0);
