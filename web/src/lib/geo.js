// Convierte metros del escenario (x, y) a [lat, lon], invirtiendo la proyeccion de tools/distrito.py
export function makeToLatLng(meta) {
  const k = meta.R * Math.cos((meta.lat0 * Math.PI) / 180);
  const deg = 180 / Math.PI;
  return (x, y) => [meta.lat0 + ((y + meta.oy) / meta.R) * deg, meta.lon0 + ((x + meta.ox) / k) * deg];
}

// Inversa de makeToLatLng: convierte [lat, lon] a metros enteros (x, y) del escenario
export function makeToXY(meta) {
  const k = meta.R * Math.cos((meta.lat0 * Math.PI) / 180);
  const rad = Math.PI / 180;
  return (lat, lon) => ({
    x: Math.round((lon - meta.lon0) * rad * k - meta.ox),
    y: Math.round((lat - meta.lat0) * rad * meta.R - meta.oy),
  });
}

// Punto dentro de un anillo (rayo horizontal). ring: lista de {x, y}
export function insideRing(p, ring) {
  let inside = false;
  for (let i = 0, j = ring.length - 1; i < ring.length; j = i++) {
    const a = ring[i];
    const b = ring[j];
    if (a.y > p.y !== b.y > p.y && p.x < ((b.x - a.x) * (p.y - a.y)) / (b.y - a.y) + a.x) inside = !inside;
  }
  return inside;
}

// Valida un cliente nuevo: dentro del distrito y fuera de todo edificio. Devuelve el motivo o null si es valido
export function clientError(p, scenario, meta) {
  const district = meta.district.map(([x, y]) => ({ x, y }));
  if (!insideRing(p, district)) return 'El punto está fuera del distrito.';
  const hit = scenario.polygons.find((poly) => insideRing(p, poly.vertices));
  if (hit) return `El punto cae dentro del obstáculo ${hit.id}: elige un lugar libre.`;
  return null;
}
