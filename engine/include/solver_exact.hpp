#pragma once
#include "scenario.hpp"
#include "visibility.hpp"

// Solver exacto (branch and bound + DP de recargas) para una base y un dron.
// Visita todos los clientes. Si se agota el tiempo, devuelve la mejor ruta encontrada.
// Si se pasa 'incumbent' (ruta del heuristico, FEASIBLE con una sola ruta), la busqueda
// arranca con su costo como cota superior y la devuelve si el B&B no la mejora.
Solution solveExact(const Scenario& sc, const VisibilityGraph& g, double timeLimitMs = 900.0,
                    const Solution* incumbent = nullptr);
