// main.cc - High-performance Monte Carlo examples
#include <iostream>
#include <chrono>
#include <random>
#include <thread>
#include <algorithm>
#include <cmath>
#include <vector>
#include "monte_carlo_engine.h"

using namespace std;
using namespace mc_engine;

// Pi estimation simulation
uint64_t pi_simulation(uint64_t n_samples, unsigned int seed) {
    std::mt19937_64 gen(seed);
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    uint64_t count = 0;
    for (uint64_t i = 0; i < n_samples; ++i) {
        double x = dist(gen);
        double y = dist(gen);
        if (x * x + y * y <= 1.0) ++count;
    }
    return count;
}

// Poker hand simulation (example of counting simulation)
uint64_t poker_simulation(uint64_t n_samples, unsigned int seed) {
    std::mt19937_64 gen(seed);
    std::uniform_int_distribution<int> card_dist(0, 51);
    
    uint64_t flush_count = 0;
    
    for (uint64_t i = 0; i < n_samples; ++i) {
        // Draw 5 cards
        int suits[4] = {0};
        std::vector<int> drawn_cards;
        drawn_cards.reserve(5);
        
        // Simple card drawing (without replacement)
        for (int j = 0; j < 5; ++j) {
            int card;
            do {
                card = card_dist(gen);
            } while (std::find(drawn_cards.begin(), drawn_cards.end(), card) != drawn_cards.end());
            
            drawn_cards.push_back(card);
            suits[card / 13]++;
        }
        
        // Check for flush (all same suit)
        for (int suit = 0; suit < 4; ++suit) {
            if (suits[suit] == 5) {
                flush_count++;
                break;
            }
        }
    }
    
    return flush_count;
}

int main(int argc, char* argv[]) {
    uint64_t total_samples = 10000000;
    if (argc > 1) total_samples = stoull(argv[1]);
    
    cout << "=== High-Performance Monte Carlo Engine ===\n";
    cout << "Samples: " << total_samples << "\n";
    cout << "Threads: " << thread::hardware_concurrency() << "\n\n";
    
    // Pi estimation with specialized method
    {
        auto start = chrono::high_resolution_clock::now();
        double pi_est = quick_pi_estimation(pi_simulation, total_samples);
        auto end = chrono::high_resolution_clock::now();
        auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);
        
        cout << "Pi estimation: " << pi_est << " (took " << duration.count() << "ms)\n";
        cout << "Error: " << abs(pi_est - M_PI) << "\n\n";
    }
    
    // Poker simulation with ratio calculation
    {
        auto start = chrono::high_resolution_clock::now();
        auto engine = make_monte_carlo_engine(poker_simulation);
        double flush_probability = engine.execute_ratio(total_samples);
        auto end = chrono::high_resolution_clock::now();
        auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);
        
        cout << "Poker flush probability: " << (flush_probability * 100) << "% (took " << duration.count() << "ms)\n";
        cout << "Theoretical: ~0.196%\n\n";
    }
    
    return 0;
}