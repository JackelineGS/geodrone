#pragma once
#include <string>
#include <vector>

#include "scenario.hpp"

struct PlannerOptions {
    double step = 250;            // separacion de la malla (metros)
    double serviceSeconds = 30;   // servicio asumido por entrega (Tabla 2)
};

struct PlannedStation {
    int id = 0;
    Point p{0, 0};
    int gain = 0;  // puntos nuevos de la malla que cubre al elegirse
};

struct PlanResult {
    double radius = 0;             // (A - kh*s) / (2*kappa)
    double step = 0;
    std::vector<Point> grid;       // puntos del distrito fuera de obstaculos
    std::vector<char> isCovered;   // cobertura de cada punto de la malla
    int covered = 0;
    std::vector<PlannedStation> stations;
    std::vector<Point> uncovered;
};

// Set Cover (greedy) para ubicar estaciones: universo = malla del distrito,
// candidatos = la misma malla, cobertura por camino minimo, y cada estacion
// debe quedar conectada a la red de recarga de la base.
PlanResult planStations(const Scenario& sc, const std::vector<Point>& district, const PlannerOptions& opt);

// Copia del escenario con las estaciones planificadas agregadas.
Scenario withStations(const Scenario& sc, const PlanResult& r);

// Contorno del distrito: "DISTRICT n" seguido de n lineas "x y".
bool readDistrict(const std::string& path, std::vector<Point>& out, std::string& err);

std::string planJson(const PlanResult& r);
