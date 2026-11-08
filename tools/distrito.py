#!/usr/bin/env python3
"""Genera el escenario del distrito (metros enteros) a partir de zonas.csv y distrito.csv.

Salidas:
  data/distrito.txt         escenario para el motor (base + poligonos, sin clientes ni estaciones)
  data/distrito/meta.json   proyeccion inversa, tipos de zona y contorno del distrito (para el mapa)
"""
import argparse
import csv
import json
import math
import sys

R = 6371000.0
MARGEN = 100  # metros de holgura para que todas las coordenadas sean enteros >= 0
AUTONOMIA = {"Medium": 3000, "Large": 4000, "Extreme": 5000}  # Tabla 2


# ---------- lectura ----------
def leer_zonas(ruta):
    zonas, orden = {}, []
    with open(ruta, newline="", encoding="utf-8-sig") as f:
        lector = csv.reader(f)
        next(lector, None)
        for n, fila in enumerate(lector, start=2):
            if not fila or not any(c.strip() for c in fila):
                continue
            try:
                if len(fila) > 3 and fila[3].strip():
                    lat, lon = float(fila[2]), float(fila[3])
                else:  # formato "lat, lon" dentro de una sola celda
                    lat, lon = (float(v) for v in fila[2].split(","))
            except ValueError:
                sys.exit(f"{ruta}: fila {n} con coordenadas invalidas: {fila}")
            nombre, tipo = fila[0].strip(), fila[1].strip()
            if nombre not in zonas:
                zonas[nombre] = {"tipo": tipo, "pts": []}
                orden.append(nombre)
            zonas[nombre]["pts"].append((lat, lon))
    return zonas, orden


def leer_distrito(ruta):
    pts = []
    with open(ruta, newline="", encoding="utf-8-sig") as f:
        lector = csv.reader(f)
        next(lector, None)
        for fila in lector:
            if len(fila) >= 2 and fila[0].strip():
                pts.append((float(fila[0]), float(fila[1])))
    if len(pts) < 3:
        sys.exit(f"{ruta}: el contorno necesita al menos 3 puntos")
    return pts


# ---------- geometria entera ----------
def cross(a, b, c):
    return (b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0])


def area2(P):
    return sum(P[i][0] * P[(i + 1) % len(P)][1] - P[(i + 1) % len(P)][0] * P[i][1] for i in range(len(P)))


def en_segmento(a, b, p):
    return cross(a, b, p) == 0 and min(a[0], b[0]) <= p[0] <= max(a[0], b[0]) and min(a[1], b[1]) <= p[1] <= max(a[1], b[1])


def cruce_propio(a, b, c, d):
    return cross(a, b, c) * cross(a, b, d) < 0 and cross(c, d, a) * cross(c, d, b) < 0


def ubicar(P, p):
    """'dentro', 'borde' o 'fuera' (aritmetica exacta con enteros)."""
    n = len(P)
    if any(en_segmento(P[i], P[(i + 1) % n], p) for i in range(n)):
        return "borde"
    dentro = False
    for i in range(n):
        a, b = P[i], P[(i + 1) % n]
        if (a[1] > p[1]) != (b[1] > p[1]):
            c = cross(a, b, p)
            if (c > 0) if b[1] > a[1] else (c < 0):
                dentro = not dentro
    return "dentro" if dentro else "fuera"


def es_simple(P):
    n = len(P)
    for i in range(n):
        for j in range(i + 1, n):
            if j == i + 1 or (i == 0 and j == n - 1):
                continue
            if cruce_propio(P[i], P[(i + 1) % n], P[j], P[(j + 1) % n]):
                return False
    return True


def convex_hull(pts):
    """Andrew (monotone chain); devuelve antihorario."""
    pts = sorted(set(pts))
    if len(pts) <= 2:
        return pts
    inf, sup = [], []
    for p in pts:
        while len(inf) >= 2 and cross(inf[-2], inf[-1], p) <= 0:
            inf.pop()
        inf.append(p)
    for p in reversed(pts):
        while len(sup) >= 2 and cross(sup[-2], sup[-1], p) <= 0:
            sup.pop()
        sup.append(p)
    return inf[:-1] + sup[:-1]


def solapan(A, B):
    if any(ubicar(B, p) == "dentro" for p in A) or any(ubicar(A, p) == "dentro" for p in B):
        return True
    return any(
        cruce_propio(A[i], A[(i + 1) % len(A)], B[j], B[(j + 1) % len(B)])
        for i in range(len(A)) for j in range(len(B))
    )


# ---------- principal ----------
def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--zonas", default="data/distrito/zonas.csv")
    ap.add_argument("--distrito", default="data/distrito/distrito.csv")
    ap.add_argument("--salida", default="data/distrito.txt")
    ap.add_argument("--meta", default="data/distrito/meta.json")
    ap.add_argument("--contorno", default="data/distrito/contorno.txt", help="contorno del distrito para el motor")
    ap.add_argument("--A", type=float, default=None, help="autonomia (por defecto, la de la Tabla 2)")
    ap.add_argument("--seed", type=int, default=2026)
    args = ap.parse_args()

    zonas, orden = leer_zonas(args.zonas)
    distrito = leer_distrito(args.distrito)
    bases = [n for n in orden if zonas[n]["tipo"].lower() == "base"]
    if len(bases) != 1:
        sys.exit(f"Se esperaba exactamente 1 zona de tipo Base y hay {len(bases)}")
    nombre_base = bases[0]

    # Proyeccion plana alrededor del centro del distrito
    lat0 = (min(p[0] for p in distrito) + max(p[0] for p in distrito)) / 2
    lon0 = (min(p[1] for p in distrito) + max(p[1] for p in distrito)) / 2

    def proy(lat, lon):
        return (math.radians(lon - lon0) * R * math.cos(math.radians(lat0)), math.radians(lat - lat0) * R)

    crudo = {n: [proy(*p) for p in zonas[n]["pts"]] for n in orden}
    crudo_d = [proy(*p) for p in distrito]
    todos = [p for v in crudo.values() for p in v] + crudo_d
    ox = min(p[0] for p in todos) - MARGEN
    oy = min(p[1] for p in todos) - MARGEN

    def entero(p):
        return (round(p[0] - ox), round(p[1] - oy))

    avisos, errores = [], []

    # Base: centroide de sus vertices
    cb = crudo[nombre_base]
    base = entero((sum(p[0] for p in cb) / len(cb), sum(p[1] for p in cb) / len(cb)))

    # Zonas: limpiar, orientar y comprobar
    poligonos = []  # (nombre, tipo, vertices)
    for n in orden:
        if n == nombre_base:
            continue
        pts = []
        for p in map(entero, crudo[n]):
            if not pts or pts[-1] != p:
                pts.append(p)
        if len(pts) > 1 and pts[0] == pts[-1]:
            pts.pop()
        if len(pts) < 3:
            errores.append(f"{n}: menos de 3 vertices distintos")
            continue
        if not es_simple(pts):
            if len(pts) <= 6:
                pts = convex_hull(pts)
                avisos.append(f"{n}: el contorno se cruzaba; se reordeno con convex hull")
            else:
                errores.append(f"{n}: el contorno se cruza y tiene {len(pts)} vertices; revisa el orden de los puntos")
                continue
        if area2(pts) < 0:
            pts.reverse()
            avisos.append(f"{n}: estaba en sentido horario; se invirtio")
        if area2(pts) == 0:
            errores.append(f"{n}: area cero")
            continue
        poligonos.append((n, zonas[n]["tipo"], pts))

    d_int = [entero(p) for p in crudo_d]

    for i in range(len(poligonos)):
        for j in range(i + 1, len(poligonos)):
            if solapan(poligonos[i][2], poligonos[j][2]):
                errores.append(f"Se superponen: {poligonos[i][0]} y {poligonos[j][0]}")
    for n, _, pts in poligonos:
        if ubicar(pts, base) != "fuera":
            errores.append(f"La base ({nombre_base}) cae dentro de {n} o sobre su borde")
        fuera = sum(1 for p in pts if ubicar(d_int, p) == "fuera")
        if fuera:
            avisos.append(f"{n}: {fuera} de {len(pts)} vertices quedan fuera del contorno del distrito")
    if ubicar(d_int, base) == "fuera":
        avisos.append("La base queda fuera del contorno del distrito")

    if errores:
        print("ERRORES (no se genero el escenario):")
        for e in errores:
            print("  -", e)
        sys.exit(1)

    maxc = max(max(p[0] for p in d_int), max(p[1] for p in d_int),
               *(max(max(p[0], p[1]) for p in pts) for _, _, pts in poligonos))
    if maxc <= 5000:
        categoria = "Medium"
    elif maxc <= 10000:
        categoria = "Large"
    elif maxc <= 20000:
        categoria = "Extreme"
    else:
        sys.exit(f"El distrito mide {maxc} m: excede el rango de Extreme (20000)")
    A = args.A if args.A is not None else AUTONOMIA[categoria]

    # Escenario para el motor (sin clientes ni estaciones; se agregan despues)
    lineas = [
        f"SEED {args.seed}",
        f"CATEGORY {categoria}",
        f"0 0 {len(poligonos)}",
        f"A {A:g} KAPPA 1 V 10 Q 10 TAU 60 KH 0.2",
        f"BASE {base[0]} {base[1]}",
    ]
    for k, (_, _, pts) in enumerate(poligonos, start=1):
        lineas.append(f"POLYGON {k} {len(pts)}")
        lineas += [f"{x} {y}" for x, y in pts]
    with open(args.salida, "w") as f:
        f.write("\n".join(lineas) + "\n")

    meta = {
        "lat0": lat0, "lon0": lon0, "R": R, "ox": ox, "oy": oy,
        "category": categoria, "A": A,
        "base": {"name": nombre_base, "x": base[0], "y": base[1]},
        "zones": [{"id": k, "name": n, "tipo": t, "vertices": [list(p) for p in pts]}
                  for k, (n, t, pts) in enumerate(poligonos, start=1)],
        "district": [list(p) for p in d_int],
    }
    with open(args.meta, "w") as f:
        json.dump(meta, f, ensure_ascii=False)

    with open(args.contorno, "w") as f:  # contorno en el formato del motor: DISTRICT n + n lineas "x y"
        f.write(f"DISTRICT {len(d_int)}\n" + "\n".join(f"{x} {y}" for x, y in d_int) + "\n")

    print(f"Categoria {categoria}: coordenadas hasta {maxc} m, A = {A:g}")
    print(f"Base ({nombre_base}): ({base[0]}, {base[1]})")
    print(f"{len(poligonos)} zonas, {sum(len(p) for _, _, p in poligonos)} vertices en total; contorno del distrito: {len(d_int)} puntos")
    for a in avisos:
        print("  aviso:", a)
    print(f"Escrito: {args.salida}, {args.meta} y {args.contorno}")


if __name__ == "__main__":
    main()
