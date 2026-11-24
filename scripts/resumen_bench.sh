#!/usr/bin/env bash
export LC_ALL=C
# Genera tablas Markdown a partir de docs/resultados_con_gap.csv
# Uso: bash scripts/resumen_bench.sh [csv] > docs/resumen_bench.md
CSV=${1:-docs/resultados_con_gap.csv}

echo "**Por categoría y algoritmo**"
echo
echo "| Categoría | Algoritmo | Filas | Tiempo prom. (ms) | Tiempo máx. (ms) | Memoria máx. (MB) | Parcial (SI) | Gap prom. (%) | Gap máx. (%) |"
echo "|---|---|---|---|---|---|---|---|---|"
awk -F, 'NR>1 {k=$2"|"$9; n[k]++; t[k]+=$8; if($8+0>mt[k])mt[k]=$8+0; if($10+0>mm[k])mm[k]=$10+0; if($12=="SI")p[k]++;
  if($21!=""){g[k]+=$21; ng[k]++; if($21+0>mg[k])mg[k]=$21+0}}
  END{for(k in n){split(k,a,"|"); o=(a[1]=="Small")?1:(a[1]=="Medium")?2:(a[1]=="Large")?3:4;
  printf "%d|%s|| %s | %s | %d | %.0f | %.0f | %.1f | %d | %.3f | %.3f |\n", o, a[2], a[1], a[2], n[k], t[k]/n[k], mt[k], mm[k], p[k], (ng[k]?g[k]/ng[k]:0), mg[k]}}' "$CSV" | sort -t'|' -k1,1n -k2,2 | cut -d'|' -f4-
echo
echo "**Gap del heurístico según el tipo de referencia**"
echo
echo "| Referencia | Casos | Gap prom. (%) | Gap máx. (%) |"
echo "|---|---|---|---|"
awk -F, 'NR>1 && $9=="heur" && $21!="" {k=$20; n[k]++; g[k]+=$21; if($21+0>m[k])m[k]=$21+0}
  END{for(k in n) printf "| %s | %d | %.3f | %.3f |\n", k, n[k], g[k]/n[k], m[k]}' "$CSV" | sort
echo
echo "**Gap del heurístico por tipo de caso**"
echo
echo "| Tipo | Casos | Gap prom. (%) | Gap máx. (%) |"
echo "|---|---|---|---|"
awk -F, 'NR>1 && $9=="heur" && $21!="" {k=$15; n[k]++; g[k]+=$21; if($21+0>m[k])m[k]=$21+0}
  END{for(k in n) printf "| %s | %d | %.3f | %.3f |\n", k, n[k], g[k]/n[k], m[k]}' "$CSV" | sort
echo
echo "**Veredictos**"
echo
echo "| Veredicto | Filas |"
echo "|---|---|"
awk -F, 'NR>1 {n[$13]++} END{for(k in n) printf "| %s | %d |\n", k, n[k]}' "$CSV" | sort
