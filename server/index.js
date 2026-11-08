const express = require('express');
const { execFile } = require('child_process');
const fs = require('fs/promises');
const path = require('path');
const crypto = require('crypto');

const SOLVER = path.resolve(__dirname, '../bin/main_solver');
const PLANNER = path.resolve(__dirname, '../bin/main_planner');
const DATA_DIR = path.resolve(__dirname, '../data');
const TMP_DIR = path.resolve(__dirname, 'tmp');
const NAME_RE = /^[A-Za-z0-9_.-]+\.txt$/; // sin barras: evita salir de data/

const app = express();
app.use(express.text({ type: '*/*', limit: '5mb' }));

// Lista los escenarios disponibles en data/ (sin las salidas de referencia)
app.get('/api/scenarios', async (_req, res) => {
  try {
    const files = await fs.readdir(DATA_DIR);
    res.json(files.filter((f) => NAME_RE.test(f) && !f.endsWith('_salida.txt')).sort());
  } catch (e) {
    res.status(500).json({ error: e.message });
  }
});

// Devuelve el texto de un escenario
app.get('/api/scenarios/:name', async (req, res) => {
  const { name } = req.params;
  if (!NAME_RE.test(name)) return res.status(400).json({ error: 'nombre invalido' });
  try {
    res.type('text/plain').send(await fs.readFile(path.join(DATA_DIR, name), 'utf8'));
  } catch {
    res.status(404).json({ error: 'escenario no encontrado' });
  }
});

// Resuelve un escenario: el cuerpo es el texto del archivo de entrada
app.post('/api/solve', async (req, res) => {
  const text = typeof req.body === 'string' ? req.body : '';
  if (!text.trim()) return res.status(400).json({ error: 'cuerpo vacio: se esperaba el escenario como texto' });

  await fs.mkdir(TMP_DIR, { recursive: true });
  const file = path.join(TMP_DIR, `${Date.now()}-${crypto.randomBytes(4).toString('hex')}.txt`);
  await fs.writeFile(file, text);

  execFile(SOLVER, [file], { timeout: 15000, maxBuffer: 10 * 1024 * 1024 }, async (error, stdout, stderr) => {
    await fs.unlink(file).catch(() => {});

    if (error && error.code === 'ENOENT') {
      return res.status(500).json({ error: 'No existe bin/main_solver: compila el motor primero' });
    }
    if (error && error.killed) {
      return res.status(504).json({ error: 'El solver excedio el tiempo limite', stderr });
    }
    // El solver sale con codigo 2 si es INFEASIBLE y con 1 si la entrada es invalida:
    // en ambos casos imprime STATUS, asi que se devuelve igual.
    if (error && !stdout.includes('STATUS')) {
      return res.status(500).json({ error: error.message, stderr });
    }
    res.json({ output: stdout, stderr, exitCode: error ? error.code : 0 });
  });
});

// Informe de alcanzabilidad (JSON generado por el motor con --reach)
app.post('/api/reach', async (req, res) => {
  const text = typeof req.body === 'string' ? req.body : '';
  if (!text.trim()) return res.status(400).json({ error: 'cuerpo vacio: se esperaba el escenario como texto' });

  await fs.mkdir(TMP_DIR, { recursive: true });
  const file = path.join(TMP_DIR, `${Date.now()}-${crypto.randomBytes(4).toString('hex')}.txt`);
  await fs.writeFile(file, text);

  execFile(SOLVER, ['--reach', file], { timeout: 15000, maxBuffer: 10 * 1024 * 1024 }, async (error, stdout, stderr) => {
    await fs.unlink(file).catch(() => {});
    if (error && error.code === 'ENOENT') {
      return res.status(500).json({ error: 'No existe bin/main_solver: compila el motor primero' });
    }
    try {
      res.json(JSON.parse(stdout));
    } catch {
      res.status(500).json({ error: error ? error.message : 'salida invalida del motor', stderr });
    }
  });
});

// Autonomia minima factible (busqueda binaria en el motor, --min-autonomy)
app.post('/api/min-autonomy', async (req, res) => {
  const text = typeof req.body === 'string' ? req.body : '';
  if (!text.trim()) return res.status(400).json({ error: 'cuerpo vacio: se esperaba el escenario como texto' });

  await fs.mkdir(TMP_DIR, { recursive: true });
  const file = path.join(TMP_DIR, `${Date.now()}-${crypto.randomBytes(4).toString('hex')}.txt`);
  await fs.writeFile(file, text);

  execFile(SOLVER, ['--min-autonomy', file], { timeout: 15000, maxBuffer: 10 * 1024 * 1024 }, async (error, stdout, stderr) => {
    await fs.unlink(file).catch(() => {});
    if (error && error.code === 'ENOENT') {
      return res.status(500).json({ error: 'No existe bin/main_solver: compila el motor primero' });
    }
    try {
      res.json(JSON.parse(stdout));
    } catch {
      res.status(500).json({ error: error ? error.message : 'salida invalida del motor', stderr });
    }
  });
});

// Metadatos del distrito (proyeccion inversa, tipos de zona, contorno) para el mapa real
app.get('/api/meta/:name', async (req, res) => {
  const { name } = req.params;
  if (!/^[A-Za-z0-9_-]+$/.test(name)) return res.status(400).json({ error: 'nombre invalido' });
  // distrito_estaciones.txt usa los metadatos de distrito/: se prueba el nombre y luego el prefijo
  for (const carpeta of [name, name.split('_')[0]]) {
    try {
      return res.type('application/json').send(await fs.readFile(path.join(DATA_DIR, carpeta, 'meta.json'), 'utf8'));
    } catch {
      /* se prueba el siguiente */
    }
  }
  res.status(404).json({ error: 'sin metadatos' });
});

// Planificador de estaciones (Set Cover): recibe el escenario como texto y devuelve el plan y el escenario con estaciones
app.post('/api/plan-stations', async (req, res) => {
  const text = typeof req.body === 'string' ? req.body : '';
  const district = String(req.query.district || '');
  if (!text.trim()) return res.status(400).json({ error: 'cuerpo vacio: se esperaba el escenario como texto' });
  if (!/^[A-Za-z0-9_-]+$/.test(district)) return res.status(400).json({ error: 'distrito invalido' });

  const contorno = path.join(DATA_DIR, district, 'contorno.txt');
  try {
    await fs.access(contorno);
  } catch {
    return res.status(404).json({ error: `No hay contorno para "${district}": ejecuta tools/distrito.py` });
  }

  const step = Math.min(1000, Math.max(50, Number(req.query.step) || 250));
  await fs.mkdir(TMP_DIR, { recursive: true });
  const id = `${Date.now()}-${crypto.randomBytes(4).toString('hex')}`;
  const inFile = path.join(TMP_DIR, `${id}.txt`);
  const outFile = path.join(TMP_DIR, `${id}-plan.txt`);
  await fs.writeFile(inFile, text);

  execFile(
    PLANNER,
    [inFile, contorno, '--step', String(step), '--out', outFile],
    { timeout: 60000, maxBuffer: 10 * 1024 * 1024 },
    async (error, stdout, stderr) => {
      let scenario = '';
      try {
        scenario = await fs.readFile(outFile, 'utf8');
      } catch {
        /* sin salida: se informa abajo */
      }
      await fs.unlink(inFile).catch(() => {});
      await fs.unlink(outFile).catch(() => {});

      if (error && error.code === 'ENOENT') {
        return res.status(500).json({ error: 'No existe bin/main_planner: compila el motor primero' });
      }
      if (error && error.killed) {
        return res.status(504).json({ error: 'El planificador excedio el tiempo limite', stderr });
      }
      try {
        res.json({ ...JSON.parse(stdout), scenario, stderr });
      } catch {
        res.status(500).json({ error: error ? error.message : 'salida invalida del motor', stderr });
      }
    }
  );
});

app.listen(3001, '127.0.0.1', () => {
  console.log('API de GeoDrone escuchando en http://127.0.0.1:3001');
});
