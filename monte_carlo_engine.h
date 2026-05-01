#ifndef MONTE_CARLO_ENGINE_H
#define MONTE_CARLO_ENGINE_H

#include <cstdint>
#include <thread>
#include <vector>
#include <future>
#include <random>
#include <type_traits>
#include <functional>
#include <numeric>

namespace mc_engine {

// Cache optimization constants
static constexpr size_t CACHE_LINE_SIZE = 64;
static constexpr size_t MIN_SAMPLES_PER_THREAD = 10000;

// Function traits for automatic type deduction
template<typename F>
struct function_traits;

template<typename R, typename... Args>
struct function_traits<R(*)(Args...)> {
    using return_type = R;
};

template<typename F>
struct function_traits : function_traits<decltype(&F::operator())> {};

template<typename C, typename R, typename... Args>
struct function_traits<R(C::*)(Args...) const> {
    using return_type = R;
};

template<typename C, typename R, typename... Args>
struct function_traits<R(C::*)(Args...)> {
    using return_type = R;
};

// Cache-aligned result storage — T must fit within one cache line
template<typename T>
struct alignas(CACHE_LINE_SIZE) CacheAlignedResult {
    static_assert(sizeof(T) < CACHE_LINE_SIZE, "T is too large for a single cache line");
    T value{};
    char padding[CACHE_LINE_SIZE - sizeof(T)];
};

// High-performance Monte Carlo Engine
template<typename SimulationFunction>
class MonteCarloEngine {
public:
    using ResultType = typename function_traits<SimulationFunction>::return_type;
    
    explicit MonteCarloEngine(SimulationFunction &&simulation_func, 
                             unsigned int num_threads = 0)
        : simulation_(std::forward<SimulationFunction>(simulation_func))
        , num_threads_(optimize_thread_count(num_threads))
        , results_(num_threads_)
    {}

    // Main execution method - automatically aggregates results
    template<typename AggregateOp = std::plus<ResultType>>
    ResultType execute(uint64_t total_samples, AggregateOp aggregate_op = {}) {
        // Optimize thread count based on workload
        auto const effective_threads = optimize_threads_for_workload(total_samples);
        
        if (effective_threads == 1) {
            // Single-threaded execution for small workloads
            std::random_device rd;
            return simulation_(total_samples, rd());
        }
        
        // Multi-threaded execution
        execute_parallel(total_samples, effective_threads);
        
        // Aggregate results
        ResultType result{};
        for (unsigned int i = 0; i < effective_threads; ++i) {
            result = aggregate_op(result, results_[i].value);
        }
        
        return result;
    }
    
    // Specialized method for averaging simulations
    double execute_average(uint64_t total_samples) {
        static_assert(std::is_arithmetic_v<ResultType>, "Average requires arithmetic type");

        auto sum = execute(total_samples, std::plus<ResultType>{});
        return static_cast<double>(sum) / static_cast<double>(total_samples);
    }
    
    // Specialized method for counting simulations (like pi estimation)
    double execute_ratio(uint64_t total_samples) {
        static_assert(std::is_integral_v<ResultType>, "Ratio requires integral type");
        
        auto total_count = execute(total_samples, std::plus<ResultType>{});
        return static_cast<double>(total_count) / static_cast<double>(total_samples);
    }
    
    // pi estimation convenience method
    double execute_pi_estimation(uint64_t total_samples) {
        return 4.0 * execute_ratio(total_samples);
    }

private:
    SimulationFunction simulation_;
    unsigned int num_threads_;
    std::vector<CacheAlignedResult<ResultType>> results_;
    
    unsigned int optimize_thread_count(unsigned int requested_threads) const {
        if (requested_threads == 0) {
            requested_threads = std::thread::hardware_concurrency();
        }
        return std::max(1u, std::min(requested_threads, 
                                   static_cast<unsigned int>(std::thread::hardware_concurrency())));
    }
    
    unsigned int optimize_threads_for_workload(uint64_t total_samples) const {
        // Don't use more threads than we have samples
        auto max_useful_threads = total_samples / MIN_SAMPLES_PER_THREAD;
        return std::min(num_threads_, static_cast<unsigned int>(std::max(1ull, max_useful_threads)));
    }
    
    void execute_parallel(uint64_t total_samples, unsigned int effective_threads) {
        const uint64_t samples_per_thread = total_samples / effective_threads;
        const uint64_t remaining_samples = total_samples % effective_threads;
        
        std::vector<std::future<ResultType>> futures;
        futures.reserve(effective_threads);
        
        // Use a single random device for seed generation
        std::random_device rd;
        
        // Launch threads
        for (unsigned int i = 0; i < effective_threads; ++i) {
            uint64_t const thread_samples = samples_per_thread + (i < remaining_samples ? 1 : 0);
            unsigned const int seed = generate_thread_seed(rd, i);
            
            futures.emplace_back(
                std::async(std::launch::async, [this, thread_samples, seed]() {
                    return simulation_(thread_samples, seed);
                })
            );
        }
        
        // Collect results
        for (unsigned int i = 0; i < effective_threads; ++i) {
            results_[i].value = futures[i].get();
        }
    }
    
    unsigned int generate_thread_seed(std::random_device& rd, unsigned int thread_id) const noexcept {
        // Simple but effective seed mixing
        return rd() ^ (thread_id * 0x9e3779b9u) ^ (thread_id << 16);
    }
};

// Factory function for easy engine creation
template<typename F>
auto make_monte_carlo_engine(F&& simulation_func, unsigned int threads = 0) {
    return MonteCarloEngine<std::decay_t<F>>(std::forward<F>(simulation_func), threads);
}

// Convenience function for quick execution
template<typename F>
auto quick_monte_carlo(F&& simulation_func, uint64_t samples, unsigned int threads = 0) {
    auto engine = make_monte_carlo_engine(std::forward<F>(simulation_func), threads);
    return engine.execute(samples);
}

// Convenience function for pi estimation
template<typename F>
double quick_pi_estimation(F&& simulation_func, uint64_t samples, unsigned int threads = 0) {
    auto engine = make_monte_carlo_engine(std::forward<F>(simulation_func), threads);
    return engine.execute_pi_estimation(samples);
}

} // namespace mc_engine

#endif