import { resolveRoute } from '../lib/parser';

const COLORS = ['#f59e0b', '#7c3aed', '#0891b2', '#db2777', '#65a30d'];

export default function MapView({ scenario, solution }) {
  // Todos los puntos, para calcular los limites del dibujo
  const pts = [
    scenario.base,
    ...scenario.clients,
    ...scenario.stations,
    ...scenario.polygons.flatMap((p) => p.vertices),
  ];
  const xs = pts.map((p) => p.x);
  const ys = pts.map((p) => p.y);
  const minX = Math.min(...xs);
  const maxX = Math.max(...xs);
  const minY = Math.min(...ys);
  const maxY = Math.max(...ys);

  const span = Math.max(maxX - minX, maxY - minY, 1);
  const u = span / 30; // unidad de tamaño para iconos y textos
  const pad = u * 3;

  // Invertir el eje Y: en SVG crece hacia abajo
  const sy = (y) => maxY + minY - y;
  const vb = [minX - pad, minY - pad, maxX - minX + 2 * pad, maxY - minY + 2 * pad].join(' ');

  const routes = solution ? solution.routes.map((ids) => resolveRoute(scenario, ids)) : [];

  const label = { fontSize: u * 0.7, textAnchor: 'middle', fontFamily: 'sans-serif' };

  return (
    <svg viewBox={vb} className="map" role="img" aria-label="Mapa de la misión">
      <rect x={minX - pad} y={minY - pad} width={maxX - minX + 2 * pad} height={maxY - minY + 2 * pad} fill="#f8fafc" />

      {/* Obstáculos */}
      {scenario.polygons.map((poly) => (
        <g key={`poly${poly.id}`}>
          <polygon
            points={poly.vertices.map((v) => `${v.x},${sy(v.y)}`).join(' ')}
            fill="rgba(220,38,38,0.25)"
            stroke="#dc2626"
            strokeWidth={u / 10}
          />
          {poly.vertices.map((v, i) => (
            <text key={i} x={v.x} y={sy(v.y) - u * 0.4} {...label} fontSize={u * 0.5} fill="#991b1b">
              {`W${poly.id}.${i + 1}`}
            </text>
          ))}
        </g>
      ))}

      {/* Rutas: una por color */}
      {routes.map((route, k) => {
        const color = COLORS[k % COLORS.length];
        return (
          <g key={`route${k}`}>
            {route.length > 1 && (
              <polyline
                points={route.map((p) => `${p.x},${sy(p.y)}`).join(' ')}
                fill="none"
                stroke={color}
                strokeWidth={u / 6}
                strokeLinejoin="round"
                strokeLinecap="round"
              />
            )}
            {route
              .filter((p) => p.type === 'waypoint')
              .map((p, i) => (
                <circle key={`w${i}`} cx={p.x} cy={sy(p.y)} r={u * 0.3} fill="#fff" stroke={color} strokeWidth={u / 8} />
              ))}
          </g>
        );
      })}

      {/* Estaciones de recarga: triángulo verde */}
      {scenario.stations.map((s) => {
        const cx = s.x;
        const cy = sy(s.y);
        const r = u * 0.9;
        return (
          <g key={`s${s.id}`}>
            <title>{`Estación S${s.id}`}</title>
            <polygon points={`${cx},${cy - r} ${cx - r},${cy + r * 0.8} ${cx + r},${cy + r * 0.8}`} fill="#16a34a" stroke="#14532d" strokeWidth={u / 10} />
            <text x={cx} y={cy + r * 0.55} {...label} fontSize={u * 0.6} fill="#fff">{`S${s.id}`}</text>
          </g>
        );
      })}

      {/* Clientes: círculo azul (relleno = obligatorio, hueco = opcional) */}
      {scenario.clients.map((c) => (
        <g key={`c${c.id}`}>
          <title>{`Cliente C${c.id} · prioridad ${c.prio} · ${c.mandatory ? 'obligatorio' : 'opcional'}`}</title>
          <circle cx={c.x} cy={sy(c.y)} r={u * 0.9} fill={c.mandatory ? '#2563eb' : '#fff'} stroke="#1e3a8a" strokeWidth={u / 8} />
          <text x={c.x} y={sy(c.y) + u * 0.25} {...label} fill={c.mandatory ? '#fff' : '#1e3a8a'}>{`C${c.id}`}</text>
        </g>
      ))}

      {/* Base: cuadrado oscuro */}
      <g>
        <title>Base logística</title>
        <rect x={scenario.base.x - u} y={sy(scenario.base.y) - u} width={u * 2} height={u * 2} fill="#111827" stroke="#000" strokeWidth={u / 10} />
        <text x={scenario.base.x} y={sy(scenario.base.y) + u * 0.25} {...label} fill="#fff">B</text>
      </g>
    </svg>
  );
}
