#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>

using Clock = std::chrono::steady_clock;
constexpr int N = 10'000'000;

struct Particle {
    float x, y, z;
    float vx, vy, vz;
    std::uint32_t color;
};

struct Particles {
    std::vector<float> x, y, z, vx, vy, vz;
    std::vector<std::uint32_t> color;

    explicit Particles(int n) : x(n), y(n), z(n), vx(n), vy(n), vz(n), color(n) {}
};

void step(std::vector<Particle>& particles) {
    for (auto& p : particles) {
        p.x += p.vx;
        p.y += p.vy;
        p.z += p.vz;
    }
}

void step(Particles& p) {
    for (std::size_t i = 0; i < p.x.size(); ++i) {
        p.x[i] += p.vx[i];
        p.y[i] += p.vy[i];
        p.z[i] += p.vz[i];
    }
}

template<class T>
double measure(T& particles) {
    auto start = Clock::now();
    step(particles);
    return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

int main() {
    std::vector<Particle> aos(N);
    Particles soa(N);
    for (int i = 0; i < N; ++i) {
        float x = i % 100, y = i % 70, z = i % 50;
        aos[i] = {x, y, z, 1, 2, 3, 0xff};
        soa.x[i] = x;
        soa.y[i] = y;
        soa.z[i] = z;
        soa.vx[i] = 1;
        soa.vy[i] = 2;
        soa.vz[i] = 3;
        soa.color[i] = 0xff;
    }

    step(aos);
    step(soa);
    std::cout << "run  AoS(ms)  SoA(ms)\n" << std::fixed << std::setprecision(3);
    for (int run = 1; run <= 5; ++run) {
        double a, s;
        if (run % 2 == 1) {
            a = measure(aos);
            s = measure(soa);
        } else {
            s = measure(soa);
            a = measure(aos);
        }
        std::cout << run << "  " << a << "  " << s << '\n';
    }

    for (int i = 0; i < N; ++i) {
        if (aos[i].x != soa.x[i] || aos[i].y != soa.y[i] || aos[i].z != soa.z[i]
            || soa.x[i] != i % 100 + 6 || soa.y[i] != i % 70 + 12 || soa.z[i] != i % 50 + 18) {
            std::cerr << "Mismatch at particle " << i << '\n';
            return 1;
        }
    }
    std::cout << N << " particles OK\n";
}
