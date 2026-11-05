import { useEffect, useState } from 'react';
import { parseScenario, parseSolution } from './lib/parser';
import MapView from './components/MapView';
import './App.css';

async function getText(url) {
  const r = await fetch(url);
  if (!r.ok) throw new Error(`${url}: ${r.status}`);
  return r.text();
}

export default function App() {
  const [names, setNames] = useState(['ejemplo.txt']);
  const [name, setName] = useState('ejemplo.txt');
  const [text, setText] = useState('');
  const [scenario, setScenario] = useState(null);
  const [solution, setSolution] = useState(null);
  const [stderr, setStderr] = useState('');
  const [note, setNote] = useState('');
  const [error, setError] = useState('');
  const [busy, setBusy] = useState(false);

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
        setScenario(parseScenario(t));
      })
      .catch((e) => !cancelled && setError(String(e)));
    return () => {
      cancelled = true;
    };
  }, [name]);

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
        <MapView scenario={scenario} solution={solution} />

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
