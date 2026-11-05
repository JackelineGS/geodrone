#pragma once
#include <iosfwd>
#include <string>
#include <vector>

#include "geometry.hpp"

struct DroneParams {
    double autonomy = 0, kappa = 1, v = 1, Q = 0, tau = 0, kh = 0;
};

struct Client {
    int id = 0;
    Point p{0, 0};
    int prio = 0;
    bool mandatory = true;
    long long e = 0, l = 0, s = 0, demand = 0;  // ventana [e,l], servicio s, demanda
};

struct Station {
    int id = 0;
    Point p{0, 0};
};

struct Scenario {
    long long seed = 0;
    std::string category;
    DroneParams drone;
    int baseId = 1;
    Point base{0, 0};
    std::vector<Client> clients;
    std::vector<Station> stations;
    std::vector<Polygon> polygons;
};

struct Solution {
    std::string status = "INFEASIBLE";  // FEASIBLE | INFEASIBLE
    double cost = 0;
    long long timeMs = 0;
    int recharges = 0;
    int skipped = 0;
    std::vector<std::vector<std::string>> routes;  // una ruta por viaje o dron: B, C#, S#, W#.#
};

// Lee un escenario. Devuelve false y llena err si el formato es inválido.
bool readScenario(std::istream& in, Scenario& sc, std::string& err);
bool readScenarioFile(const std::string& path, Scenario& sc, std::string& err);

// Comprueba coherencia: polígonos antihorarios y ningún nodo dentro de un obstáculo.
bool validateScenario(const Scenario& sc, std::string& err);

// Escribe la solución en el formato de la sección 3.2.
void writeSolution(std::ostream& out, const Solution& sol);
