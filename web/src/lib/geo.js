// Convierte metros del escenario (x, y) a [lat, lon], invirtiendo la proyeccion de tools/distrito.py
export function makeToLatLng(meta) {
  const k = meta.R * Math.cos((meta.lat0 * Math.PI) / 180);
  const deg = 180 / Math.PI;
  return (x, y) => [meta.lat0 + ((y + meta.oy) / meta.R) * deg, meta.lon0 + ((x + meta.ox) / k) * deg];
}
