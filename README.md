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

double pi = quick_pi_estimation(pi_sim, 1'000'000'000);
// → 3.14159, error 2.8e-6, ~0.9s wall time on 10 cores
```

## Design notes

- **Cache-line alignment** — each thread's result is padded to 64 bytes to prevent false sharing between cores.
- **Type-deduced aggregation** — `function_traits` infers the return type at compile time; `static_assert` enforces correct usage of `execute_ratio` vs `execute_average`.
- **Adaptive thread count** — threads are capped at `total_samples / 10,000` to avoid spawning threads whose overhead would exceed their work.
- **Independent RNG seeds** — thread seeds are mixed with `thread_id * 0x9e3779b9` (Knuth hash) to prevent correlated random streams.

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
