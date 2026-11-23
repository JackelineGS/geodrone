#include "solver_heur.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <queue>
#include <random>
#include <string>
#include <vector>

#include "route_path.hpp"
#include "shortest_path.hpp"

namespace {

const double INF = std::numeric_limits<double>::infinity();
const double EPS = 1e-9;
const int TOPR = 6;  // cuantos puntos de recarga cercanos se consideran al recargar

// Estado del dron mientras recorre un orden de clientes
struct State {
    int pos;     // nodo actual del grafo (0 = base, 1..k clientes, k+1.. estaciones)
    double bat;  // bateria restante
};

class Heur {
public:
    Heur(const Scenario& sc, const VisibilityGraph& g, double limitMs, unsigned long long seed)
        : g_(g), limitMs_(limitMs), rng_(seed), t0_(Clock::now()) {
        k_ = (int)sc.clients.size();
        m_ = (int)sc.stations.size();
        n_ = 1 + k_ + m_;
        R_ = 1 + m_;
        kappa_ = sc.drone.kappa;
        kh_ = sc.drone.kh;
        A_ = sc.drone.autonomy;

        // Paso 1: distancias minimas entre todos los nodos de interes (igual que el exacto)
        for (int i = 0; i < n_; i++) sp_.push_back(dijkstra(g, i));
        D_.assign(n_, std::vector<double>(n_, INF));
        for (int i = 0; i < n_; i++)
            for (int j = 0; j < n_; j++) D_[i][j] = sp_[i].dist[j];

        svc_.assign(k_ + 1, 0.0);
        for (int u = 1; u <= k_; u++) svc_[u] = kh_ * (double)sc.clients[u - 1].s;

        buildNetwork();  // Paso 2: red de recarga conectada a la base
        buildEntries();  // Paso 3: mejor forma de "entrar" a cada cliente tras recargar
    }

    Solution run() {
        Solution sol;
        if (!allServable_) {
            sol.timeMs = std::llround(elapsedMs());
            return sol;  // INFEASIBLE
        }
        std::vector<int> order = nearestNeighbor();
        if ((int)order.size() != k_) {
            sol.timeMs = std::llround(elapsedMs());
            return sol;
        }
        double cost = evaluate(order, nullptr);
        improve(order, cost);

        std::vector<int> stops;
        double finalCost = evaluate(order, &stops);

        std::vector<std::string> route{"B"};
        for (size_t t = 0; t + 1 < stops.size(); t++) appendPath(stops[t], stops[t + 1], route);
        if (route.size() == 1) route.push_back("B");

        int rech = 0;
        for (size_t t = 1; t + 1 < stops.size(); t++) {
            NodeType ty = g_.nodes[stops[t]].type;
            if (ty == NodeType::BASE || ty == NodeType::STATION) rech++;
        }
        sol.status = "FEASIBLE";
        sol.cost = finalCost;
        sol.recharges = rech;
        sol.skipped = 0;
        sol.routes = {route};
        sol.timeMs = std::llround(elapsedMs());
        return sol;
    }

private:
    using Clock = std::chrono::steady_clock;

    const VisibilityGraph& g_;
    double limitMs_;
    std::mt19937_64 rng_;
    Clock::time_point t0_;
    int k_, m_, n_, R_;
    double kappa_, kh_, A_;
    std::vector<ShortestPaths> sp_;
    std::vector<std::vector<double>> D_;
    std::vector<double> svc_;

    std::vector<char> inNet_;                 // punto de recarga conectado a la base
    std::vector<std::vector<double>> netD_;   // costo minimo r -> r2 saltando entre puntos de recarga
    std::vector<std::vector<int>> nxt_;       // siguiente salto en ese camino
    std::vector<double> nearD_;               // distancia al punto de recarga (de la red) mas cercano
    std::vector<std::vector<int>> near_;      // por nodo: los TOPR puntos de recarga mas cercanos
    std::vector<double> E_;                   // E_[r*(k+1)+u]: costo de ir de r a u recargando antes
    std::vector<int> entry_;                  // punto de recarga desde el que se vuela a u
    bool allServable_ = true;

    double elapsedMs() const { return std::chrono::duration<double, std::milli>(Clock::now() - t0_).count(); }
    bool timeUp() const { return elapsedMs() > limitMs_; }
    int rn(int r) const { return r == 0 ? 0 : k_ + r; }  // indice de grafo del punto de recarga r

    // ---------- Paso 2 ----------
    void buildNetwork() {
        inNet_.assign(R_, 0);
        auto hop = [&](int a, int b) {
            double d = D_[rn(a)][rn(b)];
            return d < INF && kappa_ * d <= A_ + EPS;
        };
        // DFS/BFS desde la base: que estaciones se pueden alcanzar saltando con una carga
        std::queue<int> q;
        inNet_[0] = 1;
        q.push(0);
        while (!q.empty()) {
            int r = q.front();
            q.pop();
            for (int r2 = 0; r2 < R_; r2++)
                if (!inNet_[r2] && hop(r, r2)) {
                    inNet_[r2] = 1;
                    q.push(r2);
                }
        }
        // Floyd-Warshall solo sobre los puntos de recarga (R es chico)
        netD_.assign(R_, std::vector<double>(R_, INF));
        nxt_.assign(R_, std::vector<int>(R_, -1));
        for (int a = 0; a < R_; a++) {
            if (!inNet_[a]) continue;
            netD_[a][a] = 0;
            nxt_[a][a] = a;
            for (int b = 0; b < R_; b++)
                if (a != b && inNet_[b] && hop(a, b)) {
                    netD_[a][b] = D_[rn(a)][rn(b)];
                    nxt_[a][b] = b;
                }
        }
        for (int t = 0; t < R_; t++)
            for (int a = 0; a < R_; a++) {
                if (netD_[a][t] == INF) continue;
                for (int b = 0; b < R_; b++) {
                    double v = netD_[a][t] + netD_[t][b];
                    if (v < netD_[a][b]) {
                        netD_[a][b] = v;
                        nxt_[a][b] = nxt_[a][t];
                    }
                }
            }
        // distancia de cada nodo a la red y sus TOPR recargas mas cercanas
        nearD_.assign(n_, INF);
        near_.assign(n_, {});
        for (int a = 0; a < n_; a++) {
            std::vector<int> cand;
            for (int r = 0; r < R_; r++)
                if (inNet_[r] && D_[a][rn(r)] < INF) cand.push_back(r);
            std::sort(cand.begin(), cand.end(), [&](int x, int y) { return D_[a][rn(x)] < D_[a][rn(y)]; });
            if (!cand.empty()) nearD_[a] = D_[a][rn(cand[0])];
            if ((int)cand.size() > TOPR) cand.resize(TOPR);
            near_[a] = cand;
        }
    }

    // ---------- Paso 3 ----------
    void buildEntries() {
        E_.assign((size_t)R_ * (k_ + 1), INF);
        entry_.assign((size_t)R_ * (k_ + 1), -1);
        for (int u = 1; u <= k_; u++) {
            for (int r2 = 0; r2 < R_; r2++) {
                if (!inNet_[r2]) continue;
                double d = D_[rn(r2)][u];
                if (d == INF) continue;
                // desde r2 con bateria llena: ir, atender y aun poder llegar a una recarga
                if (kappa_ * d + svc_[u] + kappa_ * nearD_[u] > A_ + EPS) continue;
                for (int r = 0; r < R_; r++) {
                    if (netD_[r][r2] == INF) continue;
                    double v = netD_[r][r2] + d;
                    size_t id = (size_t)r * (k_ + 1) + u;
                    if (v < E_[id]) {
                        E_[id] = v;
                        entry_[id] = r2;
                    }
                }
            }
            if (E_[(size_t)0 * (k_ + 1) + u] == INF) allServable_ = false;  // cliente inalcanzable
        }
    }

    // Agrega los nodos del camino a -> b: puntos de paso (W) y el destino
    void appendPath(int a, int b, std::vector<std::string>& out) const {
        appendPathLabels(g_, sp_[a], a, b, out);
    }

    // Agrega a stops el salto entre puntos de recarga r -> r2 (sin repetir r)
    void pushChain(int r, int r2, std::vector<int>* stops) const {
        int cur = r;
        while (cur != r2) {
            cur = nxt_[cur][r2];
            stops->push_back(rn(cur));
        }
    }

    // ---------- Paso 4: atender un cliente ----------
    // Devuelve el costo agregado, o INF si no se puede. Actualiza el estado.
    double step(State& st, int u, std::vector<int>* stops) const {
        const double dd = D_[st.pos][u];
        if (dd < INF) {
            double need = kappa_ * dd + svc_[u];
            // vuelo directo: debe sobrar bateria para llegar luego a una recarga
            if (st.bat - need >= kappa_ * nearD_[u] - EPS) {
                st.bat -= need;
                st.pos = u;
                if (stops) stops->push_back(u);
                return dd;
            }
        }
        // hay que recargar antes: ir a un punto de recarga cercano y de ahi al cliente
        double best = INF;
        int br = -1;
        for (int r : near_[st.pos]) {
            double d1 = D_[st.pos][rn(r)];
            if (kappa_ * d1 > st.bat + EPS) continue;
            double e = E_[(size_t)r * (k_ + 1) + u];
            if (e == INF) continue;
            if (d1 + e < best) {
                best = d1 + e;
                br = r;
            }
        }
        if (br < 0) return INF;
        int r2 = entry_[(size_t)br * (k_ + 1) + u];
        if (stops) {
            if (rn(br) != st.pos) stops->push_back(rn(br));
            pushChain(br, r2, stops);
            stops->push_back(u);
        }
        st.bat = A_ - (kappa_ * D_[rn(r2)][u] + svc_[u]);
        st.pos = u;
        return best;
    }

    // volver a la base al final
    double finish(State& st, std::vector<int>* stops) const {
        double dd = D_[st.pos][0];
        if (dd < INF && kappa_ * dd <= st.bat + EPS) {
            if (stops) stops->push_back(0);
            return dd;
        }
        double best = INF;
        int br = -1;
        for (int r : near_[st.pos]) {
            double d1 = D_[st.pos][rn(r)];
            if (kappa_ * d1 > st.bat + EPS) continue;
            if (netD_[r][0] == INF) continue;
            if (d1 + netD_[r][0] < best) {
                best = d1 + netD_[r][0];
                br = r;
            }
        }
        if (br < 0) return INF;
        if (stops) {
            if (rn(br) != st.pos) stops->push_back(rn(br));
            pushChain(br, 0, stops);
        }
        return best;
    }

    // Costo total de visitar los clientes en este orden (INF si no es posible)
    double evaluate(const std::vector<int>& order, std::vector<int>* stops) const {
        State st{0, A_};
        if (stops) {
            stops->clear();
            stops->push_back(0);
        }
        double cost = 0;
        for (int u : order) {
            double c = step(st, u, stops);
            if (c == INF) return INF;
            cost += c;
        }
        double c = finish(st, stops);
        if (c == INF) return INF;
        return cost + c;
    }

    // ---------- Paso 5: construccion greedy ----------
    std::vector<int> nearestNeighbor() {
        State st{0, A_};
        std::vector<char> used(k_ + 1, 0);
        std::vector<int> order;
        for (int t = 0; t < k_; t++) {
            int bu = -1;
            double bc = INF;
            for (int u = 1; u <= k_; u++) {
                if (used[u]) continue;
                State tmp = st;
                double c = step(tmp, u, nullptr);
                if (c < bc) {
                    bc = c;
                    bu = u;
                }
            }
            if (bu < 0) return {};
            step(st, bu, nullptr);
            used[bu] = 1;
            order.push_back(bu);
        }
        return order;
    }

    // ---------- Paso 6: mejora local ----------
    void improve(std::vector<int>& order, double& cost) {
        if (k_ < 2) return;
        // Fase A: 2-opt determinista (primera mejora)
        bool improved = true;
        while (improved && !timeUp()) {
            improved = false;
            for (int i = 0; i + 1 < k_ && !timeUp(); i++)
                for (int j = i + 1; j < k_; j++) {
                    std::reverse(order.begin() + i, order.begin() + j + 1);
                    double c = evaluate(order, nullptr);
                    if (c < cost - EPS) {
                        cost = c;
                        improved = true;
                    } else {
                        std::reverse(order.begin() + i, order.begin() + j + 1);
                    }
                }
        }
        // Fase B: busqueda local aleatoria (2-opt, mover segmento, intercambio)
        std::vector<int> cand;
        long long sinMejora = 0;                        // intentos seguidos sin mejorar
        const long long maxSinMejora = 3000LL * k_;     // parada por estancamiento
        while (!timeUp() && sinMejora < maxSinMejora) {
            for (int rep = 0; rep < 32; rep++) {
                cand = order;
                int type = (int)(rng_() % 3);
                int i = (int)(rng_() % k_), j = (int)(rng_() % k_);
                if (i > j) std::swap(i, j);
                if (type == 0) {
                    std::reverse(cand.begin() + i, cand.begin() + j + 1);
                } else if (type == 1) {
                    int len = 1 + (int)(rng_() % 3);
                    if (i + len > k_) len = k_ - i;
                    std::vector<int> seg(cand.begin() + i, cand.begin() + i + len);
                    cand.erase(cand.begin() + i, cand.begin() + i + len);
                    int p = (int)(rng_() % (cand.size() + 1));
                    cand.insert(cand.begin() + p, seg.begin(), seg.end());
                } else {
                    std::swap(cand[i], cand[j]);
                }
                double c = evaluate(cand, nullptr);
                if (c < cost - EPS) {
                    cost = c;
                    order = cand;
                    sinMejora = 0;
                } else {
                    sinMejora++;
                }
            }
        }
    }
};

}  // namespace

Solution solveHeuristic(const Scenario& sc, const VisibilityGraph& g, double timeLimitMs, unsigned long long seed) {
    Heur h(sc, g, timeLimitMs, seed);
    return h.run();
}
