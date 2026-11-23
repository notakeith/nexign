#include <atomic>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <functional>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>

#ifndef CACHE_LINE_SIZE
#define CACHE_LINE_SIZE 64
#endif

using Counter = std::atomic<std::uint64_t>;
constexpr int iterations = 100'000'000;

struct alignas(CACHE_LINE_SIZE) AdjacentCounters {
    Counter first{0}, second{0};
};

struct alignas(CACHE_LINE_SIZE) PaddedCounter {
    Counter value{0};
};

static_assert(2 * sizeof(Counter) <= CACHE_LINE_SIZE);

void increment(Counter& counter) {
    for (int i = 0; i < iterations; ++i) {
        counter.fetch_add(1, std::memory_order_relaxed);
    }
}

bool measure(Counter& a, Counter& b) {
    using Clock = std::chrono::steady_clock;
    const auto cpu = std::clock();
    const auto start = Clock::now();
    std::thread first(increment, std::ref(a));
    std::thread second(increment, std::ref(b));
    first.join();
    second.join();
    const double ms = std::chrono::duration<double, std::milli>(Clock::now() - start).count();
    const double cpu_ms = 1000.0 * (std::clock() - cpu) / CLOCKS_PER_SEC;
    std::cout << std::fixed << std::setprecision(2) << "wall_ms=" << ms
              << " cpu_ms=" << cpu_ms << " a=" << &a << " b=" << &b
              << " counts=" << a.load() << "/" << b.load() << '\n';
    return a.load() == iterations && b.load() == iterations;
}

int main(int argc, char** argv) {
    if (argc == 2 && std::string(argv[1]) == "adjacent") {
        AdjacentCounters c;
        return measure(c.first, c.second) ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "padded") {
        PaddedCounter c[2];
        return measure(c[0].value, c[1].value) ? 0 : 1;
    }
    std::cerr << "Usage: bench adjacent|padded\n";
    return 1;
}
