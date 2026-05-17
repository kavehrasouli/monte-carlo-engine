# Monte Carlo Engine

Header-only C++17 engine for parallelized Monte Carlo simulations. Write the inner loop; the engine handles threading, result aggregation, and hardware-aware scheduling.

## Quick start

```cpp
#include "monte_carlo_engine.h"
using namespace mc_engine;

uint64_t pi_sim(uint64_t n, unsigned int seed) {
    std::mt19937_64 gen(seed);
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    uint64_t hits = 0;
    for (uint64_t i = 0; i < n; ++i) {
        double x = dist(gen), y = dist(gen);
        if (x*x + y*y <= 1.0) ++hits;
    }
    return hits;
}

auto engine = make_monte_carlo_engine(pi_sim);
double pi = 4.0 * engine.execute_ratio(1'000'000'000);
// → 3.14159, error 2.8e-6, ~0.9s wall time on 10 cores
```

## Benchmarks

10-core machine, `-O3 -march=native -flto`:

| Task | Samples | Wall time | Result |
|---|---|---|---|
| π estimation | 1B | 873 ms | 3.14159 (err: 2.8e-6) |
| Poker flush probability | 1B | 7.6 s | 0.197% (theory: 0.196%) |

## Build

```bash
make           # release (-O3, LTO)
make debug     # with AddressSanitizer + UBSan
make benchmark
```
