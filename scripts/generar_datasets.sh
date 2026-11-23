#!/usr/bin/env bash
# Genera de forma reproducible todos los datasets de data/gen/ (RF13).
# Cada linea: nombre  categoria  semilla  tipo.  Misma semilla -> mismo archivo.
# Uso (desde la raiz del proyecto):
#   g++ -std=c++17 -O2 -Iengine/include engine/src/main_generator.cpp $SRC -o bin/main_generator
#   bash scripts/generar_datasets.sh
# Variables opcionales: GEN=ruta_al_generador  OUT=carpeta_de_salida
set -u
GEN=${GEN:-bin/main_generator}
OUT=${OUT:-data/gen}
mkdir -p "$OUT"
ERR=$(mktemp)

gen() {  # nombre categoria semilla tipo
    if "$GEN" "$2" "$3" "$4" > "$OUT/$1.txt" 2> "$ERR"; then
        if [ -s "$ERR" ]; then echo "AVISO $1: $(head -1 "$ERR")"; else echo "ok    $1"; fi
    else
        echo "FALLO $1: $(head -1 "$ERR")"
    fi
}

# --- Small: 16 aleatorios + 4 adversariales ---
for s in $(seq 1 16); do gen "small_$(printf '%02d' $s)" Small $s aleatorio; done
gen small_col Small 4 colineal
gen small_enc Small 2 encerrado
gen small_lim Small 3 limite
gen small_pas Small 5 pasillo

# --- Medium: 16 aleatorios + 4 adversariales ---
for s in $(seq 1 16); do gen "medium_$(printf '%02d' $s)" Medium $s aleatorio; done
gen medium_col Medium 4 colineal
gen medium_enc Medium 2 encerrado
gen medium_lim Medium 3 limite
gen medium_pas Medium 4 pasillo

# --- Large: 5 aleatorios + 4 adversariales ---
for s in $(seq 1 5); do gen "large_$(printf '%02d' $s)" Large $s aleatorio; done
gen large_col Large 4 colineal
gen large_enc Large 2 encerrado
gen large_lim Large 3 limite
gen large_pas Large 5 pasillo

# --- Extreme: 3 aleatorios + 4 adversariales ---
for s in $(seq 1 3); do gen "extreme_$(printf '%02d' $s)" Extreme $s aleatorio; done
gen extreme_col Extreme 4 colineal
gen extreme_enc Extreme 2 encerrado
gen extreme_lim Extreme 3 limite
gen extreme_pas Extreme 5 pasillo

rm -f "$ERR"
echo "Listo: $(ls "$OUT"/*.txt | wc -l) archivos en $OUT"
