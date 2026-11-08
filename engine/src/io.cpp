#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include "scenario.hpp"

namespace {

struct ParseError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

class Reader {
public:
    explicit Reader(std::istream& in) {
        std::string line;
        while (std::getline(in, line)) {
            auto c = line.find("//");
            if (c != std::string::npos) line.erase(c);
            std::istringstream ss(line);
            std::string t;
            while (ss >> t) tok_.push_back(t);
        }
    }

    const std::string& next(const char* what) {
        if (pos_ >= tok_.size()) throw ParseError(std::string("fin de archivo inesperado, se esperaba ") + what);
        return tok_[pos_++];
    }

    void expect(const char* kw) {
        const std::string& t = next(kw);
        if (t != kw) throw ParseError(std::string("se esperaba '") + kw + "' y se encontro '" + t + "'");
    }

    bool atEnd() const { return pos_ >= tok_.size(); }
    const std::string& peek() const { return tok_[pos_]; }

    long long integer(const char* what) {
        const std::string& t = next(what);
        size_t used = 0;
        long long v = 0;
        try {
            v = std::stoll(t, &used);
        } catch (...) {
            used = 0;
        }
        if (used != t.size()) throw ParseError(std::string("valor entero invalido para ") + what + ": '" + t + "'");
        return v;
    }

    double real(const char* what) {
        const std::string& t = next(what);
        size_t used = 0;
        double v = 0;
        try {
            v = std::stod(t, &used);
        } catch (...) {
            used = 0;
        }
        if (used != t.size()) throw ParseError(std::string("valor numerico invalido para ") + what + ": '" + t + "'");
        return v;
    }

private:
    std::vector<std::string> tok_;
    size_t pos_ = 0;
};

long long area2(const Polygon& p) {  // doble del area con signo
    long long s = 0;
    for (size_t i = 0; i < p.v.size(); i++) {
        const Point& a = p.v[i];
        const Point& b = p.v[(i + 1) % p.v.size()];
        s += a.x * b.y - b.x * a.y;
    }
    return s;
}

}  // namespace

bool readScenario(std::istream& in, Scenario& sc, std::string& err) {
    try {
        Reader r(in);
        sc = Scenario{};

        r.expect("SEED");
        sc.seed = r.integer("semilla");
        r.expect("CATEGORY");
        sc.category = r.next("categoria");

        long long k = r.integer("k"), m = r.integer("m"), p = r.integer("p");
        if (k < 0 || m < 0 || p < 0) throw ParseError("k, m y p deben ser >= 0");

        r.expect("A");
        sc.drone.autonomy = r.real("autonomia");
        r.expect("KAPPA");
        sc.drone.kappa = r.real("kappa");
        r.expect("V");
        sc.drone.v = r.real("v");
        r.expect("Q");
        sc.drone.Q = r.real("Q");
        r.expect("TAU");
        sc.drone.tau = r.real("tau");
        r.expect("KH");
        sc.drone.kh = r.real("kh");

        r.expect("BASE");
        sc.baseId = 1;
        sc.base.x = r.integer("base x");
        sc.base.y = r.integer("base y");

        for (long long i = 0; i < k; i++) {
            Client c;
            r.expect("CLIENT");
            c.id = (int)r.integer("id cliente");
            c.p.x = r.integer("x cliente");
            c.p.y = r.integer("y cliente");
            c.prio = (int)r.integer("prioridad");
            c.mandatory = r.integer("obligatorio") != 0;
            c.e = r.integer("e");
            c.l = r.integer("l");
            c.s = r.integer("s");
            c.demand = r.integer("demanda");
            sc.clients.push_back(c);
        }
        for (long long i = 0; i < m; i++) {
            Station s;
            r.expect("STATION");
            s.id = (int)r.integer("id estacion");
            s.p.x = r.integer("x estacion");
            s.p.y = r.integer("y estacion");
            sc.stations.push_back(s);
        }
        for (long long i = 0; i < p; i++) {
            Polygon poly;
            r.expect("POLYGON");
            poly.id = (int)r.integer("id poligono");
            long long nv = r.integer("nv");
            if (nv < 3) throw ParseError("un poligono necesita al menos 3 vertices");
            for (long long j = 0; j < nv; j++) {
                Point v;
                v.x = r.integer("x vertice");
                v.y = r.integer("y vertice");
                poly.v.push_back(v);
            }
            sc.polygons.push_back(poly);
        }
        if (!r.atEnd()) throw ParseError("hay datos sobrantes tras el ultimo poligono: '" + r.peek() + "'");
        return true;
    } catch (const ParseError& e) {
        err = e.what();
        return false;
    }
}

bool readScenarioFile(const std::string& path, Scenario& sc, std::string& err) {
    std::ifstream in(path);
    if (!in) {
        err = "no se pudo abrir: " + path;
        return false;
    }
    return readScenario(in, sc, err);
}

bool validateScenario(const Scenario& sc, std::string& err) {
    for (const Polygon& poly : sc.polygons) {
        if (area2(poly) <= 0) {
            err = "el poligono " + std::to_string(poly.id) + " no esta en sentido antihorario";
            return false;
        }
    }
    auto check = [&](const Point& p, const std::string& name) {
        for (const Polygon& poly : sc.polygons) {
            if (locatePoint(poly, p) == Location::INTERIOR) {
                err = name + " esta dentro del obstaculo " + std::to_string(poly.id);
                return false;
            }
        }
        return true;
    };
    if (!check(sc.base, "la base")) return false;
    for (const Client& c : sc.clients)
        if (!check(c.p, "el cliente C" + std::to_string(c.id))) return false;
    for (const Station& s : sc.stations)
        if (!check(s.p, "la estacion S" + std::to_string(s.id))) return false;
    return true;
}

void writeSolution(std::ostream& out, const Solution& sol) {
    out << "STATUS " << sol.status << "\n";
    if (sol.status != "FEASIBLE") return;
    out << "COST " << std::fixed << std::setprecision(3) << sol.cost << "\n";
    out << "TIME_MS " << sol.timeMs << "\n";
    out << "RECHARGES " << sol.recharges << "\n";
    out << "SKIPPED " << sol.skipped << "\n";
    for (const auto& route : sol.routes) {
        out << "ROUTE " << route.size() << "\n";
        for (size_t i = 0; i < route.size(); i++) out << (i ? " " : "") << route[i];
        out << "\n";
    }
}

void writeScenario(std::ostream& out, const Scenario& sc) {
    out << std::setprecision(10);
    out << "SEED " << sc.seed << "\n";
    out << "CATEGORY " << sc.category << "\n";
    out << sc.clients.size() << " " << sc.stations.size() << " " << sc.polygons.size() << "\n";
    out << "A " << sc.drone.autonomy << " KAPPA " << sc.drone.kappa << " V " << sc.drone.v << " Q " << sc.drone.Q
        << " TAU " << sc.drone.tau << " KH " << sc.drone.kh << "\n";
    out << "BASE " << sc.base.x << " " << sc.base.y << "\n";
    for (const Client& c : sc.clients)
        out << "CLIENT " << c.id << " " << c.p.x << " " << c.p.y << " " << c.prio << " " << (c.mandatory ? 1 : 0) << " "
            << c.e << " " << c.l << " " << c.s << " " << c.demand << "\n";
    for (const Station& s : sc.stations) out << "STATION " << s.id << " " << s.p.x << " " << s.p.y << "\n";
    for (const Polygon& poly : sc.polygons) {
        out << "POLYGON " << poly.id << " " << poly.v.size() << "\n";
        for (const Point& v : poly.v) out << v.x << " " << v.y << "\n";
    }
}
