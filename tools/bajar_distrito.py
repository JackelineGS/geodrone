#!/usr/bin/env python3
"""Descarga el limite administrativo de un distrito desde OpenStreetMap (Nominatim)
y lo guarda como CSV lat,lon simplificado."""
import csv
import json
import sys
import urllib.parse
import urllib.request

consulta = sys.argv[1] if len(sys.argv) > 1 else "La Molina, Lima, Peru"
salida = sys.argv[2] if len(sys.argv) > 2 else "data/distrito/distrito.csv"
tolerancia = 0.0003  # grados (~33 m): menos puntos, misma forma

if consulta[:1] in ("R", "r") and consulta[1:].isdigit():  # id de relacion de OSM, ej. R1944745
    url = "https://nominatim.openstreetmap.org/lookup?" + urllib.parse.urlencode(
        {"osm_ids": consulta.upper(), "format": "jsonv2", "polygon_geojson": 1}
    )
else:
    url = "https://nominatim.openstreetmap.org/search?" + urllib.parse.urlencode(
        {"q": consulta, "format": "jsonv2", "polygon_geojson": 1, "countrycodes": "pe", "limit": 5}
    )
req = urllib.request.Request(url, headers={"User-Agent": "geodrone-proyecto-universitario/1.0"})
with urllib.request.urlopen(req, timeout=30) as r:
    resultados = json.load(r)

# Primer resultado que sea un limite administrativo con poligono
elegido = next(
    (x for x in resultados
     if x.get("category") == "boundary" and x.get("geojson", {}).get("type") in ("Polygon", "MultiPolygon")),
    None,
)
if not elegido:
    sys.exit("No se encontro un limite administrativo. Resultados: " + ", ".join(x["display_name"] for x in resultados))

geo = elegido["geojson"]
anillos = [geo["coordinates"][0]] if geo["type"] == "Polygon" else [p[0] for p in geo["coordinates"]]


def area(a):
    return abs(sum(a[i][0] * a[(i + 1) % len(a)][1] - a[(i + 1) % len(a)][0] * a[i][1] for i in range(len(a)))) / 2


anillo = max(anillos, key=area)  # el anillo exterior mas grande
if anillo[0] == anillo[-1]:
    anillo = anillo[:-1]


def rdp(pts, eps):
    """Douglas-Peucker."""
    if len(pts) < 3:
        return pts
    (x1, y1), (x2, y2) = pts[0], pts[-1]
    dx, dy = x2 - x1, y2 - y1
    norma = (dx * dx + dy * dy) ** 0.5 or 1e-12
    dmax, idx = 0, 0
    for i in range(1, len(pts) - 1):
        d = abs(dy * pts[i][0] - dx * pts[i][1] + x2 * y1 - y2 * x1) / norma
        if d > dmax:
            dmax, idx = d, i
    if dmax <= eps:
        return [pts[0], pts[-1]]
    return rdp(pts[: idx + 1], eps)[:-1] + rdp(pts[idx:], eps)


# Se parte el anillo en dos mitades para poder simplificar un contorno cerrado
mitad = len(anillo) // 2
simple = rdp(anillo[: mitad + 1], tolerancia)[:-1] + rdp(anillo[mitad:] + [anillo[0]], tolerancia)[:-1]

with open(salida, "w", newline="") as f:
    w = csv.writer(f)
    w.writerow(["lat", "lon"])
    for lon, lat in simple:
        w.writerow([f"{lat:.6f}", f"{lon:.6f}"])

print(f"{elegido['display_name']}")
print(f"{len(anillo)} puntos originales -> {len(simple)} simplificados -> {salida}")
