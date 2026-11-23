# GeoDrone Challenge (Avance 2)

Motor en C++17 para planificar misiones de entrega con un dron: lee un escenario
(base, clientes, estaciones de recarga y edificios), construye un grafo de visibilidad
y calcula la ruta de menor costo respetando los edificios y la batería.

## Estructura

| `engine/include/`, `engine/src/` | Código del motor: geometría, grafo de visibilidad, caminos mínimos (Dijkstra), DFS, solver exacto (branch and bound) |
| `engine/src/main_solver.cpp` | Programa que resuelve un escenario |
| `engine/src/main_generator.cpp` | Generador de escenarios con semilla (aleatorios y adversariales) |
| `engine/src/judge.cpp` | Verificador de rutas, independiente del solver |
| `engine/src/main_bench.cpp` | Benchmark: corre varios casos y genera una tabla CSV |
| `engine/tests/` | Pruebas unitarias de cada módulo |
| `data/` | Escenarios de ejemplo; `data/gen/` guarda los generados |
| `docs/formato_entrada.txt` | Formato de los archivos de entrada |

## Compilar

Requiere `g++` con soporte de C++17 (en Windows, MSYS2 UCRT64). Desde la raíz del proyecto:

```bash
mkdir -p bin
SRC="engine/src/geometry.cpp engine/src/io.cpp engine/src/visibility.cpp engine/src/shortest_path.cpp engine/src/solver_exact.cpp engine/src/reachability.cpp engine/src/param_search.cpp engine/src/station_planner.cpp engine/src/judge.cpp"

g++ -std=c++17 -O2 -Iengine/include engine/src/main_solver.cpp    $SRC -o bin/main_solver.exe
g++ -std=c++17 -O2 -Iengine/include engine/src/main_generator.cpp $SRC -o bin/main_generator.exe
g++ -std=c++17 -O2 -Iengine/include engine/src/main_bench.cpp     $SRC -o bin/main_bench.exe
```

En Linux o macOS se puede quitar el `.exe`.

## Uso

**Resolver un escenario**

```bash
./bin/main_solver.exe data/ejemplo.txt
```

Salida esperada: `STATUS FEASIBLE`, `COST 27.605` y la ruta `B C2 C1 S1 W1.1 B`.
`B` es la base, `C#` un cliente, `S#` una estación y `W#.#` una esquina de edificio usada como punto de paso.

**Generar escenarios**

```bash
./bin/main_generator.exe <categoria> <semilla> <tipo> > archivo.txt
```

- `categoria`: `Small`, `Medium`, `Large` o `Extreme`.
- `semilla`: número entero. La misma semilla produce siempre el mismo archivo.
- `tipo`: `aleatorio`, `colineal`, `encerrado`, `limite` o `pasillo`.

Ejemplo: `./bin/main_generator.exe Small 1 aleatorio > data/gen/small_01.txt`

**Benchmark**

```bash
./bin/main_bench.exe data/gen/*.txt > resultados.csv
```

Por cada caso resuelve el escenario, verifica la ruta de forma independiente y escribe
una fila con: estado, costo, tiempo, recargas, si el solver agotó su tiempo (`parcial`) y el veredicto.

| Veredicto | Significado |
|---|---|
| AC | Ruta válida y dentro del tiempo (o infactibilidad detectada correctamente) |
| WA | La ruta no pasa el verificador |
| TLE | Excede el tiempo máximo de la categoría (Small 1 s, Medium 2 s, Large 5 s, Extreme 10 s) |
| RE | No se pudo leer el archivo |

**Pruebas unitarias** (ejemplo con la de geometría; hay una por módulo en `engine/tests/`)

```bash
g++ -std=c++17 -O2 -Iengine/include engine/tests/test_geometry.cpp $SRC -o bin/test_geometry.exe
./bin/test_geometry.exe
```

## Notas

- El solver exacto devuelve el óptimo en casos pequeños. Con muchos clientes agota su tiempo
  límite y entrega la mejor ruta encontrada (`parcial = SI`), sin garantía de óptimo.
- La carpeta `web/` y `server/` contienen una visualización opcional; no son necesarias para el motor.