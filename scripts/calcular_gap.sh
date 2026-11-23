#!/usr/bin/env bash
export LC_ALL=C
# Agrega al CSV del bench: mejor_conocido, tipo_ref, gap_pct y calidad_pct.
# Referencia de cada caso:
#   - optimo:           el exacto termino (parcial=NO) con ruta FEASIBLE y no-WA -> se usa su costo
#   - mejor_conocida:   si no, el menor costo FEASIBLE (no-WA) entre los algoritmos
#   gap_pct     = 100 * (costo - ref) / ref      (se rotula con tipo_ref: no mezclar optimo con mejor_conocida)
#   calidad_pct = 100 * ref / costo
# Uso: bash scripts/calcular_gap.sh [entrada.csv] [salida.csv]
IN=${1:-docs/resultados_bench.csv}
OUT=${2:-docs/resultados_con_gap.csv}
awk -F, '
NR == FNR {                       # primera pasada: referencia por caso
    if (FNR > 1 && $6 == "FEASIBLE" && $7 != "" && $13 != "WA") {
        c = $7 + 0
        if (!($1 in best) || c < best[$1]) best[$1] = c
        if ($9 == "exacto" && $12 == "NO") opt[$1] = c
    }
    next
}
FNR == 1 { print $0 ",mejor_conocido,tipo_ref,gap_pct,calidad_pct"; next }
{                                 # segunda pasada: agrega las columnas
    ref = ""; tipo = ""; g = ""; q = ""
    if ($1 in best) {
        if ($1 in opt) { ref = opt[$1]; tipo = "optimo" }
        else           { ref = best[$1]; tipo = "mejor_conocida" }
        if ($6 == "FEASIBLE" && $7 != "" && $13 != "WA") {
            if (ref > 0) { g = sprintf("%.3f", 100 * ($7 - ref) / ref); q = sprintf("%.3f", 100 * ref / $7) }
            else { g = "0.000"; q = "100.000" }
        }
        ref = sprintf("%.3f", ref)
    }
    print $0 "," ref "," tipo "," g "," q
}' "$IN" "$IN" > "$OUT"
echo "CSV con gap escrito en $OUT" >&2
