#include "station_planner.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <utility>

#include "shortest_path.hpp"
#include "visibility.hpp"

namespace {

const double INF = std::numeric_limits<double>::infinity();
const double EPS = 1e-9;

struct Box {
    long long x0, y0, x1, y1;
};

Box boxOf(const Polygon& poly) {
    Box b{poly.v[0].x, poly.v[0].y, poly.v[0].x, poly.v[0].y};
    for (const Point& p : poly.v) {
        b.x0 = std::min(b.x0, p.x);
        b.y0 = std::min(b.y0, p.y);
        b.x1 = std::max(b.x1, p.x);
        b.y1 = std::max(b.y1, p.y);
    }
    return b;
}

bool overlap(const Box& b, const Point& a, const Point& c) {
    return std::max(a.x, c.x) >= b.x0 && std::min(a.x, c.x) <= b.x1 &&
           std::max(a.y, c.y) >= b.y0 && std::min(a.y, c.y) <= b.y1;
}

class Planner {
public:
    Planner(const Scenario& sc, const std::vector<Point>& district, const PlannerOptions& opt)
        : sc_(sc), opt_(opt) {
        district_.v = district;
        for (const Polygon& p : sc.polygons) boxes_.push_back(boxOf(p));
    }

    PlanResult run() {
        PlanResult res;
        res.step = opt_.step;
        const double kappa = sc_.drone.kappa, A = sc_.drone.autonomy;
        res.radius = kappa > 0 ? (A - sc_.drone.kh * opt_.serviceSeconds) / (2 * kappa) : 0;
        rcov_ = res.radius;
        hop_ = kappa > 0 ? A / kappa : 0;
        kappa_ = kappa;
        A_ = A;

        makeGrid();
        n_ = (int)P_.size();
        res.grid = P_;
        res.isCovered.assign(n_, 0);
        if (n_ == 0 || kappa <= 0 || rcov_ <= 0) {
            res.uncovered = P_;
            return res;
        }

        // Nodos fijos: base, clientes y estaciones del escenario, y vertices de poligonos
        g_ = buildVisibilityGraph(sc_);
        nf_ = g_.size();
        D_.reserve(nf_);
        for (int u = 0; u < nf_; u++) D_.push_back(dijkstra(g_, u).dist);

        // Para cada punto de la malla: nodos fijos que ve y distancia minima a todos los nodos fijos
        vis_.assign(n_, {});
        F_.assign(n_, std::vector<double>(nf_, INF));
        for (int i = 0; i < n_; i++) {
            for (int u = 0; u < nf_; u++)
                if (visible(P_[i], g_.nodes[u].p)) vis_[i].push_back({u, dist(P_[i], g_.nodes[u].p)});
            for (const auto& [v, w] : vis_[i])
                for (int u = 0; u < nf_; u++) F_[i][u] = std::min(F_[i][u], w + D_[v][u]);
        }

        // Red inicial: la base y las estaciones existentes conectadas a ella
        std::vector<int> charging{0};
        for (int u = 0; u < nf_; u++)
            if (g_.nodes[u].type == NodeType::STATION) charging.push_back(u);
        std::vector<char> inNet(nf_, 0);
        std::vector<int> stack{0};
        inNet[0] = 1;
        while (!stack.empty()) {
            int a = stack.back();
            stack.pop_back();
            for (int b : charging)
                if (!inNet[b] && D_[a][b] != INF && kappa * D_[a][b] <= A + EPS) {
                    inNet[b] = 1;
                    stack.push_back(b);
                }
        }

        // Cobertura de los candidatos: cover_[c] = puntos de la malla que cubre
        cover_.assign(n_, {});
        for (int i = 0; i < n_; i++) cover_[i].push_back(i);
        for (int i = 0; i < n_; i++)
            for (int j = i + 1; j < n_; j++) {
                if (dist(P_[i], P_[j]) > rcov_ + EPS) continue;  // la distancia real nunca es menor
                if (gridDist(i, j) <= rcov_ + EPS) {
                    cover_[i].push_back(j);
                    cover_[j].push_back(i);
                }
            }

        std::vector<char>& covered = res.isCovered;
        int coveredCount = 0;
        auto mark = [&](int j) {
            if (!covered[j]) {
                covered[j] = 1;
                coveredCount++;
            }
        };
        std::vector<char> connectable(n_, 0);
        for (int u : charging) {
            if (!inNet[u]) continue;
            for (int j = 0; j < n_; j++) {
                double d = fixedDist(u, j);
                if (d <= rcov_ + EPS) mark(j);
                if (kappa * d <= A + EPS) connectable[j] = 1;
            }
        }

        // Greedy: la estacion conectada que cubre mas puntos aun sin cubrir
        std::vector<char> chosen(n_, 0);
        int nextId = 1;
        for (const Station& s : sc_.stations) nextId = std::max(nextId, s.id + 1);
        while (coveredCount < n_) {
            int best = -1, bestGain = 0;
            for (int c = 0; c < n_; c++) {
                if (!connectable[c] || chosen[c]) continue;
                int gain = 0;
                for (int j : cover_[c]) gain += !covered[j];
                if (gain > bestGain) {
                    bestGain = gain;
                    best = c;
                }
            }
            if (best < 0) break;  // nada mas se puede cubrir con estaciones conectadas
            chosen[best] = 1;
            for (int j : cover_[best]) mark(j);
            res.stations.push_back({nextId++, P_[best], bestGain});
            for (int c = 0; c < n_; c++) {
                if (connectable[c] || dist(P_[best], P_[c]) * kappa > A + EPS) continue;
                if (kappa * gridDist(best, c) <= A + EPS) connectable[c] = 1;
            }
        }

        res.covered = coveredCount;
        for (int j = 0; j < n_; j++)
            if (!covered[j]) res.uncovered.push_back(P_[j]);
        return res;
    }

private:
    const Scenario& sc_;
    PlannerOptions opt_;
    Polygon district_;
    std::vector<Box> boxes_;
    double rcov_ = 0, hop_ = 0, kappa_ = 1, A_ = 0;
    int n_ = 0, nf_ = 0;
    std::vector<Point> P_;
    VisibilityGraph g_;
    std::vector<std::vector<double>> D_;                  // D_[u][v]: camino minimo entre nodos fijos
    std::vector<std::vector<std::pair<int, double>>> vis_;  // vis_[i]: (nodo fijo, distancia) que el punto i ve
    std::vector<std::vector<double>> F_;                  // F_[i][u]: camino minimo del punto i al nodo fijo u
    std::vector<std::vector<int>> cover_;

    bool visible(const Point& a, const Point& b) const {
        for (size_t k = 0; k < sc_.polygons.size(); k++) {
            if (!overlap(boxes_[k], a, b)) continue;
            if (!segmentValid(sc_.polygons[k], a, b)) return false;
        }
        return true;
    }

    void makeGrid() {
        if (district_.v.size() < 3) return;
        long long x0 = district_.v[0].x, x1 = x0, y0 = district_.v[0].y, y1 = y0;
        for (const Point& p : district_.v) {
            x0 = std::min(x0, p.x);
            x1 = std::max(x1, p.x);
            y0 = std::min(y0, p.y);
            y1 = std::max(y1, p.y);
        }
        const double st = std::max(1.0, opt_.step);
        for (long long iy = 0; (double)y0 + iy * st <= (double)y1; iy++)
            for (long long ix = 0; (double)x0 + ix * st <= (double)x1; ix++) {
                Point p{std::llround((double)x0 + ix * st), std::llround((double)y0 + iy * st)};
                if (locatePoint(district_, p) == Location::EXTERIOR) continue;
                bool ok = true;
                for (const Polygon& poly : sc_.polygons)
                    if (locatePoint(poly, p) != Location::EXTERIOR) {
                        ok = false;
                        break;
                    }
                if (ok) P_.push_back(p);
            }
    }

    // Camino minimo entre dos puntos de la malla
    double gridDist(int i, int j) const {
        if (visible(P_[i], P_[j])) return dist(P_[i], P_[j]);
        double best = INF;
        for (const auto& [u, w] : vis_[j]) best = std::min(best, F_[i][u] + w);
        return best;
    }

    // Camino minimo entre un nodo fijo y un punto de la malla
    double fixedDist(int u, int j) const {
        double best = INF;
        for (const auto& [v, w] : vis_[j]) best = std::min(best, D_[u][v] + w);
        return best;
    }
};

}  // namespace

PlanResult planStations(const Scenario& sc, const std::vector<Point>& district, const PlannerOptions& opt) {
    return Planner(sc, district, opt).run();
}

Scenario withStations(const Scenario& sc, const PlanResult& r) {
    Scenario out = sc;
    for (const PlannedStation& s : r.stations) out.stations.push_back({s.id, s.p});
    return out;
}

bool readDistrict(const std::string& path, std::vector<Point>& out, std::string& err) {
    std::ifstream in(path);
    if (!in) {
        err = "no se pudo abrir: " + path;
        return false;
    }
    std::string kw;
    long long n = 0;
    if (!(in >> kw >> n) || kw != "DISTRICT" || n < 3) {
        err = "formato invalido: se esperaba 'DISTRICT n' con n >= 3";
        return false;
    }
    out.clear();
    for (long long i = 0; i < n; i++) {
        Point p;
        if (!(in >> p.x >> p.y)) {
            err = "faltan coordenadas en el contorno";
            return false;
        }
        out.push_back(p);
    }
    return true;
}

std::string planJson(const PlanResult& r) {
    std::ostringstream o;
    o << std::fixed << std::setprecision(1);
    o << "{\"radius\":" << r.radius << ",\"step\":" << r.step << ",\"gridPoints\":" << r.grid.size()
      << ",\"covered\":" << r.covered << ",\"uncovered\":" << r.uncovered.size() << ",\"stations\":[";
    for (size_t i = 0; i < r.stations.size(); i++) {
        const PlannedStation& s = r.stations[i];
        o << (i ? "," : "") << "{\"id\":" << s.id << ",\"x\":" << s.p.x << ",\"y\":" << s.p.y << ",\"gain\":" << s.gain
          << "}";
    }
    o << "],\"uncoveredPoints\":[";
    const size_t cap = std::min<size_t>(r.uncovered.size(), 300);
    for (size_t i = 0; i < cap; i++) o << (i ? "," : "") << "[" << r.uncovered[i].x << "," << r.uncovered[i].y << "]";
    o << "]}";
    return o.str();
}
