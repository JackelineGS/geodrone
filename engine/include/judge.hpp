#pragma once
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