import { useEffect, useMemo, useState } from 'react';
import { parseScenario, parseSolution } from './lib/parser';
import MapView from './components/MapView';
import LeafletMap from './components/LeafletMap';
import { zoneColor } from './lib/zones';
import './App.css';

async function getText(url) {
  const r = await fetch(url);
  if (!r.ok) throw new Error(`${url}: ${r.status}`);
  return r.text();
}

// Cambia el valor de A en la linea "A <autonomia> KAPPA ..." del escenario
function withAutonomy(text, a) {
  if (a === null || !text) return text;
  return text.replace(/^A\s+\S+/m, `A ${a}`);
}

export default function App() {
  const [names, setNames] = useState(['ejemplo.txt']);
  const [name, setName] = useState('ejemplo.txt');
  const [baseText, setText] = useState('');
  const [autonomy, setAutonomy] = useState(null); // null = usar la A del archivo
  const [scenario, setScenario] = useState(null);
  const [solution, setSolution] = useState(null);
  const [stderr, setStderr] = useState('');
  const [note, setNote] = useState('');
  const [error, setError] = useState('');
  const [busy, setBusy] = useState(false);
  const [reach, setReach] = useState(null);
  const [minA, setMinA] = useState(null);
  const [showReach, setShowReach] = useState(true);
  const [meta, setMeta] = useState(null);
  const [view, setView] = useState('plano');

  // Texto que se envia al motor: el del archivo, con la autonomia elegida
  const fileA = scenario ? scenario.params.A : 0;
  const currentA = autonomy ?? fileA;
  const text = useMemo(() => withAutonomy(baseText, autonomy), [baseText, autonomy]);

  // Lista de escenarios disponibles
  useEffect(() => {
    fetch('/api/scenarios')
      .then((r) => r.json())
      .then((list) => list.length && setNames(list))
      .catch(() => {});
  }, []);

  // Carga el escenario elegido (API primero, archivo estático como respaldo)
  useEffect(() => {
    let cancelled = false;
    getText(`/api/scenarios/${name}`)
      .catch(() => getText(`/data/${name}`))
      .then((t) => {
        if (cancelled) return;
        setText(t);
        setAutonomy(null);
        setScenario(parseScenario(t));
      })
      .catch((e) => !cancelled && setError(String(e)));
    return () => {
      cancelled = true;
    };
  }, [name]);

  // Alcance (seccion 3.4): lo calcula el motor con --reach
  useEffect(() => {
    if (!text) return;
    let cancelled = false;
    setReach(null);
    fetch('/api/reach', { method: 'POST', headers: { 'Content-Type': 'text/plain' }, body: text })
      .then((r) => (r.ok ? r.json() : null))
      .then((d) => !cancelled && setReach(d))
      .catch(() => !cancelled && setReach(null));
    return () => {
      cancelled = true;
    };
  }, [text]);

  // Autonomia minima (RF10): busqueda binaria en el motor
  useEffect(() => {
    if (!text) return;
    let cancelled = false;
    setMinA(null);
    fetch('/api/min-autonomy', { method: 'POST', headers: { 'Content-Type': 'text/plain' }, body: text })
      .then((r) => (r.ok ? r.json() : null))
      .then((d) => !cancelled && setMinA(d))
      .catch(() => !cancelled && setMinA(null));
    return () => {
      cancelled = true;
    };
  }, [text]);

  // Metadatos del distrito (proyeccion inversa, tipos de zona, contorno) para el mapa real
  useEffect(() => {
    let cancelled = false;
    setMeta(null);
    setView('plano');
    const base = name.replace('.txt', '');
    getText(`/api/meta/${base}`)
      .catch(() => getText(`/data/${base}_meta.json`))
      .then((t) => {
        if (cancelled) return;
        setMeta(JSON.parse(t));
        setView('mapa');
      })
      .catch(() => {});
    return () => {
      cancelled = true;
    };
  }, [name]);

  function changeA(v) {
    setAutonomy(v);
    setSolution(null); // la ruta calculada era para la A anterior
    setNote('');
  }

  function choose(newName) {
    setName(newName);
    setSolution(null);
    setStderr('');
    setNote('');
    setError('');
  }

  async function solve() {
    setBusy(true);
    setError('');
    setNote('');
    try {
      const r = await fetch('/api/solve', {
        method: 'POST',
        headers: { 'Content-Type': 'text/plain' },
        body: text,
      });
      const data = await r.json();
      if (!data.output) throw new Error(data.error || `HTTP ${r.status}`);
      setSolution(parseSolution(data.output));
      setStderr(data.stderr || '');
    } catch (e) {
      try {
        const out = await getText(`/data/${name.replace('.txt', '_salida.txt')}`);
        setSolution(parseSolution(out));
        setStderr('');
        setNote(`API no disponible (${e.message}): se muestra la salida precalculada.`);
      } catch {
        setError(`No se pudo resolver: ${e.message}`);
      }
    } finally {
      setBusy(false);
    }
  }

  if (!scenario) return <p className="msg">{error || 'Cargando…'}</p>;

  return (
    <div className="app">
      <header>
        <h1>GeoDrone Logistics</h1>
        <span>
          {scenario.category} · semilla {scenario.seed}
        </span>
        <div className="controls">
          <select value={name} onChange={(e) => choose(e.target.value)}>
            {names.map((n) => (
              <option key={n} value={n}>
                {n}
              </option>
            ))}
          </select>
          <button onClick={solve} disabled={busy}>
            {busy ? 'Resolviendo…' : 'Resolver'}
          </button>
        </div>
      </header>

      {error && <p className="msg err">{error}</p>}
      {note && <p className="msg warn">{note}</p>}

      <main>
        <div>
          {meta && (
            <div className="viewtabs">
              <button className={view === 'mapa' ? 'on' : ''} onClick={() => setView('mapa')}>Mapa</button>
              <button className={view === 'plano' ? 'on' : ''} onClick={() => setView('plano')}>Plano</button>
            </div>
          )}
          {meta && view === 'mapa' ? (
            <LeafletMap scenario={scenario} meta={meta} solution={solution} reach={showReach ? reach : null} />
          ) : (
            <MapView scenario={scenario} solution={solution} reach={showReach ? reach : null} />
          )}
        </div>

        <aside>
          <h2>Resultado</h2>
          {!solution && <p>Pulsa «Resolver» para calcular la ruta.</p>}
          {solution && (
            <>
              <p><b>Estado:</b> {solution.status}</p>
              {solution.status === 'FEASIBLE' && (
                <>
                  <p><b>Costo:</b> {solution.cost}</p>
                  <p><b>Recargas:</b> {solution.recharges}</p>
                  <p><b>Omitidos:</b> {solution.skipped}</p>
                  <p><b>Tiempo:</b> {solution.timeMs} ms</p>
                  {solution.routes.map((r, i) => (
                    <p key={i}><b>Ruta {i + 1}:</b> {r.join(' → ')}</p>
                  ))}
                </>
              )}
              {stderr && (
                <details>
                  <summary>Mensajes del motor</summary>
                  <pre>{stderr}</pre>
                </details>
              )}
            </>
          )}

          <h2>Alcance</h2>
          <div className="autonomy">
            <label>
              Autonomía A{' '}
              <input
                type="number"
                min="0"
                step="100"
                value={currentA}
                onChange={(e) => changeA(Number(e.target.value) || 0)}
              />
            </label>
            <div className="presets">
              {[1000, 2000, 3000, 4000, 5000].map((v) => (
                <button key={v} className={currentA === v ? 'on' : ''} onClick={() => changeA(v)}>
                  {v}
                </button>
              ))}
            </div>
          </div>
          {!reach && <p>Sin datos de alcance (¿API apagada?).</p>}
          {reach && (
            <>
              <label>
                <input type="checkbox" checked={showReach} onChange={(e) => setShowReach(e.target.checked)} /> Mostrar área
              </label>
              <p><b>Radio:</b> {reach.radius == null ? '—' : reach.radius.toFixed(2)}</p>
              <p><b>Red de recarga:</b> {reach.network.join(', ')}</p>
              {minA && (
                <p>
                  <b>Autonomía mínima:</b>{' '}
                  {minA.feasible ? `${minA.minAutonomy.toFixed(2)} (margen ${minA.margin.toFixed(2)})` : 'ninguna alcanza'}
                </p>
              )}
              {reach.isolated.length > 0 && <p><b>Estaciones aisladas:</b> {reach.isolated.join(', ')}</p>}
              <ul className="reach">
                {reach.clients.map((c) => (
                  <li key={c.id} className={c.reachable ? 'ok' : 'bad'}>
                    C{c.id}: {c.reachable ? (c.direct ? 'directo' : `con recarga en ${c.via}`) : 'inalcanzable'}
                  </li>
                ))}
              </ul>
            </>
          )}

          {meta && view === 'mapa' && (
            <>
              <h2>Zonas</h2>
              <ul className="legend">
                {[...new Set(meta.zones.map((z) => z.tipo))].map((t) => (
                  <li key={t}>
                    <span className="ic" style={{ background: zoneColor(t), opacity: 0.6 }} /> {t}
                  </li>
                ))}
              </ul>
            </>
          )}

          <h2>Leyenda</h2>
          <ul className="legend">
            <li><span className="ic base" /> Base</li>
            <li><span className="ic client" /> Cliente obligatorio</li>
            <li><span className="ic client opt" /> Cliente opcional</li>
            <li><span className="ic station" /> Estación de recarga</li>
            <li><span className="ic poly" /> Obstáculo</li>
            <li><span className="ic route" /> Ruta</li>
          </ul>
        </aside>
      </main>
    </div>
  );
}
