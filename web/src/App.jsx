import { useEffect, useState } from 'react';
import { parseScenario, parseSolution } from './lib/parser';
import MapView from './components/MapView';
import './App.css';

export default function App() {
  const [scenario, setScenario] = useState(null);
  const [solution, setSolution] = useState(null);
  const [error, setError] = useState('');

  useEffect(() => {
    Promise.all([
      fetch('/data/ejemplo.txt').then((r) => r.text()),
      fetch('/data/ejemplo_salida.txt').then((r) => r.text()),
    ])
      .then(([inp, out]) => {
        setScenario(parseScenario(inp));
        setSolution(parseSolution(out));
      })
      .catch((e) => setError(String(e)));
  }, []);

  if (error) return <p className="msg">Error: {error}</p>;
  if (!scenario || !solution) return <p className="msg">Cargando…</p>;

  return (
    <div className="app">
      <header>
        <h1>GeoDrone Logistics</h1>
        <span>
          {scenario.category} · semilla {scenario.seed}
        </span>
      </header>

      <main>
        <MapView scenario={scenario} solution={solution} />

        <aside>
          <h2>Resultado</h2>
          <p><b>Estado:</b> {solution.status}</p>
          <p><b>Costo:</b> {solution.cost}</p>
          <p><b>Recargas:</b> {solution.recharges}</p>
          <p><b>Tiempo:</b> {solution.timeMs} ms</p>
          <p><b>Ruta:</b> {solution.route.join(' → ')}</p>

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
