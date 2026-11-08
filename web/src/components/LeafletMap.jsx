import { useEffect, useRef } from 'react';
import L from 'leaflet';
import 'leaflet/dist/leaflet.css';
import { resolveRoute } from '../lib/parser';
import { makeToLatLng } from '../lib/geo';
import { zoneColor } from '../lib/zones';

const ROUTE_COLORS = ['#f59e0b', '#7c3aed', '#0891b2', '#db2777', '#65a30d'];

function icon(cls, text) {
  return L.divIcon({
    className: '',
    html: `<div class="mk ${cls}">${text}</div>`,
    iconSize: [28, 28],
    iconAnchor: [14, 14],
  });
}

export default function LeafletMap({ scenario, meta, solution, reach }) {
  const box = useRef(null);
  const mapRef = useRef(null);
  const layerRef = useRef(null);
  const fittedFor = useRef(null);

  // El mapa se crea una sola vez
  useEffect(() => {
    const map = L.map(box.current);
    const calles = L.tileLayer('https://tile.openstreetmap.org/{z}/{x}/{y}.png', {
      maxZoom: 19,
      attribution: '&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a>',
    }).addTo(map);
    const satelite = L.tileLayer(
      'https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/{z}/{y}/{x}',
      { maxZoom: 19, attribution: 'Tiles &copy; Esri' }
    );
    L.control.layers({ Calles: calles, Satélite: satelite }).addTo(map);
    layerRef.current = L.layerGroup().addTo(map);
    mapRef.current = map;
    return () => {
      map.remove();
      mapRef.current = null;
      layerRef.current = null;
      fittedFor.current = null;
    };
  }, []);

  // Las capas se redibujan cuando cambian los datos
  useEffect(() => {
    const map = mapRef.current;
    const group = layerRef.current;
    if (!map || !group) return;
    group.clearLayers();

    const ll = makeToLatLng(meta);
    const pt = (p) => ll(p.x, p.y);
    const bad = (id) => !!reach && reach.clients.some((c) => c.id === id && !c.reachable);

    // Contorno del distrito
    const distrito = L.polygon(
      meta.district.map(([x, y]) => ll(x, y)),
      { color: '#dc2626', weight: 2, dashArray: '6 6', fill: false, interactive: false }
    ).addTo(group);

    // Area de alcance (cota en linea recta) alrededor de la base y de cada estacion de la red
    if (reach && reach.radius != null) {
      reach.network.forEach((lab) => {
        const p = lab === 'B' ? scenario.base : scenario.stations.find((s) => `S${s.id}` === lab);
        if (!p) return;
        L.circle(pt(p), {
          radius: reach.radius,
          color: '#16a34a',
          weight: 1.5,
          dashArray: '6 6',
          fillColor: '#22c55e',
          fillOpacity: 0.08,
          interactive: false,
        }).addTo(group);
      });
    }

    // Zonas restringidas, coloreadas por tipo
    meta.zones.forEach((z) => {
      const c = zoneColor(z.tipo);
      L.polygon(
        z.vertices.map(([x, y]) => ll(x, y)),
        { color: c, weight: 2, fillColor: c, fillOpacity: 0.35 }
      )
        .bindTooltip(`${z.name} · ${z.tipo}`, { sticky: true })
        .addTo(group);
    });

    // Rutas: una por color
    (solution ? solution.routes : []).forEach((ids, k) => {
      const pts = resolveRoute(scenario, ids).map(pt);
      if (pts.length > 1) {
        L.polyline(pts, { color: ROUTE_COLORS[k % ROUTE_COLORS.length], weight: 4, opacity: 0.9 }).addTo(group);
      }
    });

    // Estaciones, clientes y base
    scenario.stations.forEach((s) => {
      L.marker(pt(s), { icon: icon('mk-station', `S${s.id}`) })
        .bindTooltip(`Estación S${s.id}`)
        .addTo(group);
    });
    scenario.clients.forEach((c) => {
      L.marker(pt(c), { icon: icon(bad(c.id) ? 'mk-client mk-bad' : 'mk-client', `C${c.id}`) })
        .bindTooltip(`Cliente C${c.id} · ${c.mandatory ? 'obligatorio' : 'opcional'}`)
        .addTo(group);
    });
    L.marker(pt(scenario.base), { icon: icon('mk-base', 'B') })
      .bindTooltip(meta.base?.name || 'Base')
      .addTo(group);

    // Encuadre inicial sobre el distrito (solo la primera vez para estos metadatos)
    if (fittedFor.current !== meta) {
      map.fitBounds(distrito.getBounds(), { padding: [10, 10] });
      fittedFor.current = meta;
    }
  }, [scenario, meta, solution, reach]);

  return <div ref={box} className="map" />;
}
