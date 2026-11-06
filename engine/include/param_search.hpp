#pragma once
#include <string>

#include "scenario.hpp"
#include "visibility.hpp"

struct MinAutonomy {
    bool feasible = false;  // false si ninguna autonomia permite atender a todos (cliente encerrado)
    double value = 0;       // menor A (con tolerancia eps) para el que la mision es factible
    int iterations = 0;
};

// Funcion de factibilidad: con autonomia A, todos los clientes son alcanzables
// (equivale a que el solver exacto encuentre ruta; ver pruebas).
bool missionFeasible(const Scenario& sc, const VisibilityGraph& g, double A);

// Busqueda binaria sobre la respuesta (RF10). Valida porque la factibilidad es monotona en A.
MinAutonomy minAutonomy(const Scenario& sc, const VisibilityGraph& g, double eps = 1e-6);

// Resultado en JSON (lo consume la API y la demo).
std::string minAutonomyJson(const Scenario& sc, const MinAutonomy& r);
