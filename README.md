# GeoDrone Challenge (Avance 2)

Motor en C++17 para planificar misiones de entrega con un dron: lee un escenario
(base, clientes, estaciones de recarga y edificios), construye un grafo de visibilidad
y calcula la ruta de menor costo respetando los edificios y la batería.
Incluye un solver exacto (branch and bound con programación dinámica de recargas), un heurístico,
un verificador independiente, un generador de casos y un benchmark reproducible.

## Estructura

| Carpeta / archivo | Contenido |
|---|---|
| `engine/include/`, `engine/src/` | Motor: geometría, grafo de visibilidad, caminos mínimos, alcanzabilidad, autonomía mínima, planificador de estaciones, solver exacto y heurístico |
| `engine/src/main_*.cpp` | Programas: `main_solver`, `main_generator`, `main_bench`, `main_planner`, `main_network` |
| `engine/src/judge.cpp` | Verificador de rutas, independiente del solver |
| `engine/tests/` | Pruebas unitarias de cada módulo |
| `server/` | Servidor Node.js que expone el motor |
| `web/` | Interfaz web (Vite) |
| `scripts/` | `generar_datasets.sh`, `correr_bench.sh`, `calcular_gap.sh`, `resumen_bench.sh` |
| `data/` | Escenarios de ejemplo; `data/gen/` guarda los generados |
| `docs/` | Formato de entrada, imagen de arquitectura y resultados del benchmark |

## Arquitectura

![Arquitectura de GeoDrone](docs/Arqui_Geodrone.jpeg)

Flujo del motor:

1. **Lectura y validación** del escenario (`io`).
2. **Grafo de visibilidad** (`visibility`): nodos de base, clientes, estaciones y vértices de edificios; una arista existe si el vuelo recto no entra al interior de ningún obstáculo.
3. **Caminos mínimos** (`shortest_path`): Dijkstra desde cada nodo de interés.
4. **Alcanzabilidad** (`reachability`): detecta clientes encerrados o fuera de la red de recarga (caso INFEASIBLE).
5. **Solvers**:
   - `solver_heur`: vecino más cercano + mejora local (2-opt y búsqueda aleatoria), reproducible por semilla.
   - `solver_exact`: branch and bound sobre el orden de clientes, con programación dinámica para elegir dónde recargar. Puede arrancar desde la ruta del heurístico como cota superior inicial.
6. **Juez** (`judge`): verifica la ruta sin usar el solver (visibilidad de cada tramo, autonomía y visita de todos los clientes).

## Compilar

Requiere `g++` con C++17 y CMake. En Ubuntu, desde la raíz del proyecto:

```bash
cmake -S engine -B engine/build -DCMAKE_BUILD_TYPE=Release
cmake --build engine/build -j
```

Los ejecutables quedan en `bin/`.

