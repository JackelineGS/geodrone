const express = require('express');
const { execFile } = require('child_process');
const fs = require('fs/promises');
const path = require('path');
const crypto = require('crypto');

const SOLVER = path.resolve(__dirname, '../bin/main_solver');
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

app.listen(3001, '127.0.0.1', () => {
  console.log('API de GeoDrone escuchando en http://127.0.0.1:3001');
});
