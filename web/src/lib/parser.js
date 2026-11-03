// Lee el archivo de entrada (escenario)
export function parseScenario(text) {
  const lines = text
    .split(/\r?\n/)
    .map((l) => l.replace(/\/\/.*$/, '').trim())
    .filter(Boolean);

  const sc = {
    seed: null,
    category: '',
    counts: null,
    params: {},
    base: null,
    clients: [],
    stations: [],
    polygons: [],
  };

  for (let i = 0; i < lines.length; i++) {
    const t = lines[i].split(/\s+/);
    const key = t[0];

    if (key === 'SEED') sc.seed = t[1];
    else if (key === 'CATEGORY') sc.category = t[1];
    else if (key === 'A') {
      // A 20 KAPPA 1 V 1 Q 10 TAU 0 KH 0 -> pares clave/valor
      for (let j = 0; j + 1 < t.length; j += 2) sc.params[t[j]] = Number(t[j + 1]);
    } else if (key === 'BASE') {
      sc.base = { x: Number(t[1]), y: Number(t[2]) };
    } else if (key === 'CLIENT') {
      sc.clients.push({
        id: Number(t[1]),
        x: Number(t[2]),
        y: Number(t[3]),
        prio: Number(t[4]),
        mandatory: t[5] === '1',
      });
    } else if (key === 'STATION') {
      sc.stations.push({ id: Number(t[1]), x: Number(t[2]), y: Number(t[3]) });
    } else if (key === 'POLYGON') {
      const id = Number(t[1]);
      const nv = Number(t[2]);
      const vertices = [];
      for (let j = 1; j <= nv; j++) {
        const [x, y] = lines[i + j].split(/\s+/).map(Number);
        vertices.push({ x, y });
      }
      sc.polygons.push({ id, vertices });
      i += nv;
    } else if (/^\d+$/.test(key) && t.length === 3) {
      sc.counts = { k: Number(t[0]), m: Number(t[1]), p: Number(t[2]) };
    }
  }
  return sc;
}

// Lee el archivo de salida del solver
export function parseSolution(text) {
  const lines = text
    .split(/\r?\n/)
    .map((l) => l.replace(/\/\/.*$/, '').trim())
    .filter(Boolean);

  const sol = { status: '', cost: null, timeMs: null, recharges: 0, skipped: 0, route: [] };

  for (let i = 0; i < lines.length; i++) {
    const t = lines[i].split(/\s+/);
    if (t[0] === 'STATUS') sol.status = t[1];
    else if (t[0] === 'COST') sol.cost = Number(t[1]);
    else if (t[0] === 'TIME_MS') sol.timeMs = Number(t[1]);
    else if (t[0] === 'RECHARGES') sol.recharges = Number(t[1]);
    else if (t[0] === 'SKIPPED') sol.skipped = Number(t[1]);
    else if (t[0] === 'ROUTE') {
      sol.route = (lines[i + 1] || '').split(/\s+/).filter(Boolean);
      i++;
    }
  }
  return sol;
}

// Convierte ["B","C2","S1","W1.1"] en puntos con coordenadas
export function resolveRoute(scenario, ids) {
  const points = [];
  for (const id of ids) {
    let p = null;
    if (id === 'B') {
      p = { ...scenario.base, type: 'base' };
    } else if (id.startsWith('C')) {
      const c = scenario.clients.find((c) => c.id === Number(id.slice(1)));
      if (c) p = { x: c.x, y: c.y, type: 'client' };
    } else if (id.startsWith('S')) {
      const s = scenario.stations.find((s) => s.id === Number(id.slice(1)));
      if (s) p = { x: s.x, y: s.y, type: 'station' };
    } else if (id.startsWith('W')) {
      const [pid, v] = id.slice(1).split('.').map(Number);
      const poly = scenario.polygons.find((q) => q.id === pid);
      const vert = poly && poly.vertices[v - 1];
      if (vert) p = { x: vert.x, y: vert.y, type: 'waypoint' };
    }
    if (p) points.push({ id, ...p });
  }
  return points;
}
