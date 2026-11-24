#pragma once

// Complejidad temporal (k clientes, m estaciones, R = m + 1 puntos de recarga, n = 1 + k + m nodos):
//   precalculo: n Dijkstra, O(n (V + E) log V).
//   B&B sobre ordenes de clientes: O(k!) en el peor caso; la poda por cota inferior y el limite
//   de tiempo lo acotan en la practica. Cada nodo del arbol cuesta O(k R^2) (capa de la DP) mas
//   O(R^3) (saltos entre recargas) en el peor caso.
// Complejidad espacial: O(n^2) por la matriz de distancias y O(k R) por la DP de recargas.
// Aplicabilidad: una base, un dron y una sola ruta; todos los clientes son obligatorios
//   (skipped = 0). Es exacto solo si termina antes del limite: con parcial = SI la ruta es valida
//   pero sin garantia de optimo. Opcionalmente arranca desde la ruta de un heuristico (incumbent).
#include "scenario.hpp"
#include "visibility.hpp"

// Solver exacto (branch and bound + DP de recargas) para una base y un dron.
// Visita todos los clientes. Si se agota el tiempo, devuelve la mejor ruta encontrada.
// Si se pasa 'incumbent' (ruta del heuristico, FEASIBLE con una sola ruta), la busqueda
// arranca con su costo como cota superior y la devuelve si el B&B no la mejora.
Solution solveExact(const Scenario& sc, const VisibilityGraph& g, double timeLimitMs = 900.0,
                    const Solution* incumbent = nullptr);
