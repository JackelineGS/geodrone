#!/usr/bin/env bash
# Agrega gap_pct y calidad_pct a cada fila del CSV del bench.
# Mejor conocido = menor costo FEASIBLE del mismo caso entre los algoritmos (sin contar filas WA).
#   gap_pct     = 100 * (costo - mejor) / mejor
#   calidad_pct = 100 * mejor / costo
# Uso: bash scripts/calcular_gap.sh [entrada.csv] [salida.csv]
IN=${1:-docs/resultados_bench.csv}
OUT=${2:-docs/resultados_con_gap.csv}
awk -F, '
NR == FNR {                       # primera pasada: mejor costo por caso
    if (FNR > 1 && $6 == "FEASIBLE" && $7 != "" && $13 != "WA")
        if (!($1 in best) || $7 + 0 < best[$1]) best[$1] = $7 + 0
    next
}
FNR == 1 { print $0 ",gap_pct,calidad_pct"; next }
{                                 # segunda pasada: agrega las columnas
    g = ""; q = ""
    if ($6 == "FEASIBLE" && $7 != "" && ($1 in best)) {
        if (best[$1] > 0) { g = sprintf("%.3f", 100 * ($7 - best[$1]) / best[$1]); q = sprintf("%.3f", 100 * best[$1] / $7) }
        else { g = "0.000"; q = "100.000" }
    }
    print $0 "," g "," q
}' "$IN" "$IN" > "$OUT"
echo "CSV con gap escrito en $OUT" >&2
