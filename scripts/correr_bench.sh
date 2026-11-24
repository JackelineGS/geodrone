#!/usr/bin/env bash
export LC_ALL=C
# Corre el bench con los dos algoritmos sobre cada caso y arma un solo CSV.
# Un proceso por caso y algoritmo, para que memoria_mb sea el pico de ese caso.
# Uso (desde la raiz del proyecto):  bash scripts/correr_bench.sh [caso1.txt caso2.txt ...]
# Sin argumentos usa data/gen/*.txt.  Variables: BENCH=bin/main_bench  OUT=docs/resultados_bench.csv
set -u
BENCH=${BENCH:-bin/main_bench}
OUT=${OUT:-docs/resultados_bench.csv}
mkdir -p "$(dirname "$OUT")"
if [ $# -gt 0 ]; then FILES=("$@"); else FILES=(data/gen/*.txt); fi

: > "$OUT"
primero=1
for f in "${FILES[@]}"; do
    for algo in exacto heur; do
        if [ $primero -eq 1 ]; then
            "$BENCH" "$algo" "$f" >> "$OUT" 2>/dev/null
            primero=0
        else
            "$BENCH" --no-header "$algo" "$f" >> "$OUT" 2>/dev/null
        fi
        echo "listo: $(basename "$f") $algo" >&2
    done
done
echo "CSV escrito en $OUT ($(($(wc -l < "$OUT") - 1)) filas)" >&2
