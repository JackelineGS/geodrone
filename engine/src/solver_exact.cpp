#include "solver_exact.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "shortest_path.hpp"

namespace {

const double INF = std::numeric_limits<double>::infinity();
const double EPS = 1e-9;

// Como se llego a un estado (capa i, punto de recarga r)
//  type 0: segmento desde (pi, pr) atendiendo los clientes pi+1..i
//  type 1: salto sin clientes desde (i, pr)
struct Parent {
    int type = -1, pi = -1, pr = -1;
};

class Search {
public:
    Search(const Scenario& sc, const VisibilityGraph& g, double limitMs)
        : g_(g), limitMs_(limitMs), t0_(Clock::now()) {
        k_ = (int)sc.clients.size();
        m_ = (int)sc.stations.size();
        n_ = 1 + k_ + m_;  // nodos de interes: base, clientes, estaciones (mismos indices que en el grafo)
        R_ = 1 + m_;       // puntos de recarga: 0 = base, 1..m = estaciones
        kappa_ = sc.drone.kappa;
        kh_ = sc.drone.kh;
        A_ = sc.drone.autonomy;

        for (int i = 0; i < n_; i++) sp_.push_back(dijkstra(g, i));
        D_.assign(n_, std::vector<double>(n_, INF));
        for (int i = 0; i < n_; i++)
            for (int j = 0; j < n_; j++) D_[i][j] = sp_[i].dist[j];

        svc_.assign(k_ + 1, 0.0);
        for (int u = 1; u <= k_; u++) svc_[u] = kh_ * (double)sc.clients[u - 1].s;

        G_.assign(k_ + 1, std::vector<double>(R_, INF));
        par_.assign(k_ + 1, std::vector<Parent>(R_));
        cnode_.assign(k_ + 2, 0);
        P_.assign(k_ + 2, 0.0);
        SV_.assign(k_ + 2, 0.0);
        used_.assign(k_ + 1, false);
    }

    Solution run() {
        Solution sol;
        bool reachable = true;
        for (int u = 1; u <= k_; u++)
            if (D_[0][u] == INF) reachable = false;  // cliente encerrado: INFEASIBLE sin buscar

        if (reachable) {
            G_[0][0] = 0.0;
            relaxHops(0);
            dfs(0);
            if (truncated_) std::cerr << "Aviso: se agoto el tiempo; la ruta puede no ser optima\n";
        }
        if (best_ < INF) {
            sol.status = "FEASIBLE";
            sol.cost = best_;
            sol.recharges = bestRecharges_;
            sol.skipped = 0;
            sol.routes = {bestRoute_};
        }
        sol.timeMs = std::llround(elapsedMs());
        return sol;
    }

private:
    using Clock = std::chrono::steady_clock;

    const VisibilityGraph& g_;
    double limitMs_;
    Clock::time_point t0_;
    int k_, m_, n_, R_;
    double kappa_, kh_, A_;
    std::vector<ShortestPaths> sp_;
    std::vector<std::vector<double>> D_;
    std::vector<double> svc_;
    std::vector<std::vector<double>> G_;  // G_[i][r]: costo minimo en el punto de recarga r tras atender c1..ci
    std::vector<std::vector<Parent>> par_;
    std::vector<int> cnode_;  // cnode_[i]: nodo del i-esimo cliente del orden
    std::vector<double> P_;   // P_[i]: largo de la cadena c1 -> ... -> ci
    std::vector<double> SV_;  // SV_[i]: consumo acumulado por servicio de c1..ci
    std::vector<char> used_;
    double best_ = INF;
    std::vector<std::string> bestRoute_;
    int bestRecharges_ = 0;
    long long nodes_ = 0;
    bool truncated_ = false;

    double elapsedMs() const { return std::chrono::duration<double, std::milli>(Clock::now() - t0_).count(); }

    int rn(int r) const { return r == 0 ? 0 : k_ + r; }  // indice de grafo del punto de recarga r

    void relaxHops(int i) {
        for (int rep = 0; rep < R_; rep++) {
            bool changed = false;
            for (int r = 0; r < R_; r++) {
                if (G_[i][r] == INF) continue;
                for (int r2 = 0; r2 < R_; r2++) {
                    if (r2 == r) continue;
                    double d = D_[rn(r)][rn(r2)];
                    if (d == INF || kappa_ * d > A_ + EPS) continue;
                    double v = G_[i][r] + d;
                    if (v < G_[i][r2]) {
                        G_[i][r2] = v;
                        par_[i][r2] = Parent{1, i, r};
                        changed = true;
                    }
                }
            }
            if (!changed) break;
        }
    }

    void computeLayer(int i) {
        for (int r = 0; r < R_; r++) {
            G_[i][r] = INF;
            par_[i][r] = Parent();
        }
        const int last = cnode_[i];
        for (int ip = 0; ip < i; ip++) {
            const double inner = P_[i] - P_[ip + 1];
            const double serv = SV_[i] - SV_[ip];
            for (int r = 0; r < R_; r++) {
                if (G_[ip][r] == INF) continue;
                const double head = D_[rn(r)][cnode_[ip + 1]];
                if (head == INF) continue;
                for (int r2 = 0; r2 < R_; r2++) {
                    const double tail = D_[last][rn(r2)];
                    if (tail == INF) continue;
                    const double len = head + inner + tail;
                    if (kappa_ * len + serv > A_ + EPS) continue;  // no alcanza la bateria
                    const double v = G_[ip][r] + len;
                    if (v < G_[i][r2]) {
                        G_[i][r2] = v;
                        par_[i][r2] = Parent{0, ip, r};
                    }
                }
            }
        }
        relaxHops(i);
    }

    void dfs(int i) {
        if (truncated_) return;
        if ((++nodes_ & 1023) == 0 && elapsedMs() > limitMs_) {
            truncated_ = true;
            return;
        }
        if (i == k_) {
            double c = G_[k_][0];
            if (c < best_ - EPS) {
                best_ = c;
                reconstruct();
            }
            return;
        }
        const int last = (i == 0) ? 0 : cnode_[i];
        std::vector<int> cand;
        for (int u = 1; u <= k_; u++)
            if (!used_[u]) cand.push_back(u);
        std::sort(cand.begin(), cand.end(), [&](int a, int b) { return D_[last][a] < D_[last][b]; });

        for (int u : cand) {
            cnode_[i + 1] = u;
            used_[u] = true;
            P_[i + 1] = (i == 0) ? 0.0 : P_[i] + D_[cnode_[i]][u];
            SV_[i + 1] = SV_[i] + svc_[u];
            computeLayer(i + 1);

            // Cota inferior: cadena sin limite de bateria + mejor regreso posible
            const double pre = D_[0][cnode_[1]] + P_[i + 1];
            double lb;
            if (i + 1 == k_) {
                lb = pre + D_[u][0];
            } else {
                double far = 0;
                for (int v = 1; v <= k_; v++)
                    if (!used_[v]) far = std::max(far, D_[u][v] + D_[v][0]);
                lb = pre + far;
            }
            if (lb < best_ - EPS) dfs(i + 1);
            used_[u] = false;
            if (truncated_) return;
        }
    }

    // Agrega los nodos del camino a -> b: puntos de paso (W) y el destino
    void appendPath(int a, int b, std::vector<std::string>& out) const {
        std::vector<int> p = reconstructPath(sp_[a], a, b);
        for (size_t t = 1; t < p.size(); t++) {
            const Node& nd = g_.nodes[p[t]];
            if (t + 1 == p.size() || nd.type == NodeType::WAYPOINT) out.push_back(nodeLabel(nd));
        }
    }

    void emit(int i, int r, std::vector<std::string>& out, int& arrivals) const {
        const Parent& p = par_[i][r];
        if (p.type == -1) {
            out.push_back("B");
            return;
        }
        if (p.type == 1) {
            emit(i, p.pr, out, arrivals);
            appendPath(rn(p.pr), rn(r), out);
            arrivals++;
            return;
        }
        emit(p.pi, p.pr, out, arrivals);
        int cur = rn(p.pr);
        for (int t = p.pi + 1; t <= i; t++) {
            appendPath(cur, cnode_[t], out);
            cur = cnode_[t];
        }
        appendPath(cur, rn(r), out);
        arrivals++;
    }

    void reconstruct() {
        std::vector<std::string> route;
        int arrivals = 0;
        emit(k_, 0, route, arrivals);
        if (route.size() == 1) route.push_back("B");
        bestRoute_ = route;
        bestRecharges_ = std::max(0, arrivals - 1);  // la ultima llegada es la base final
    }
};

}  // namespace

Solution solveExact(const Scenario& sc, const VisibilityGraph& g, double timeLimitMs) {
    Search s(sc, g, timeLimitMs);
    return s.run();
}
