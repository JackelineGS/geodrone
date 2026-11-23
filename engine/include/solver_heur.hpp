#pragma once
#include "scenario.hpp"
#include "visibility.hpp"

// Solver heuristico (RF08): vecino mas cercano + mejora local (2-opt y busqueda aleatoria).
// No garantiza el optimo, pero escala a Medium, Large y Extreme.
// Complejidad: precalculo O(n (V+E) log V) + O(R^2 k); construccion O(k^2 T);
// cada evaluacion de un orden es O(k T); la mejora se limita por tiempo.
// Reproducible: misma semilla -> mismas decisiones aleatorias.
Solution solveHeuristic(const Scenario& sc, const VisibilityGraph& g, double timeLimitMs = 1000.0,
                        unsigned long long seed = 12345);
