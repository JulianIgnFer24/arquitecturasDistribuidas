// ej4.cpp — Conteo de primos secuencial vs paralelo con asignación dinámica de bloques
// ej4.cpp — Prime counting: sequential vs parallel with dynamic block assignment

#include <atomic>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>
#include <algorithm>

using Clock = std::chrono::high_resolution_clock;
using Ms    = std::chrono::duration<double, std::milli>;

// Resumen de resultados (cantidad + top 10 mayores)
// Results summary (count + top 10 largest)
struct PrimeSummary {
    unsigned long long total = 0;
    std::vector<long long> top10_desc;
};

// Criba base hasta sqrt(N) para segmentar eficientemente
// Base sieve up to sqrt(N) for efficient segmentation
static std::vector<int> build_base_primes(long long limit) {
    if (limit < 2) return {};
    std::vector<unsigned char> is_prime(limit + 1, 1);
    is_prime[0] = is_prime[1] = 0;
    for (long long p = 2; p * p <= limit; ++p) {
        if (is_prime[(size_t)p]) {
            for (long long j = p * p; j <= limit; j += p) is_prime[(size_t)j] = 0;
        }
    }
    std::vector<int> base;
    for (long long i = 2; i <= limit; ++i) if (is_prime[(size_t)i]) base.push_back((int)i);
    return base;
}

// Secuencial simple (criba clásica)
// Simple sequential sieve (classic)
static PrimeSummary count_primes_sequential(long long N) {
    PrimeSummary out;
    if (N <= 2) return out;

    std::vector<unsigned char> is_prime((size_t)N, 1);
    is_prime[0] = 0;
    if (N > 1) is_prime[1] = 0;

    for (long long p = 2; p * p < N; ++p) {
        if (is_prime[(size_t)p]) {
            for (long long j = p * p; j < N; j += p) is_prime[(size_t)j] = 0;
        }
    }
    for (long long i = 2; i < N; ++i) if (is_prime[(size_t)i]) ++out.total;

    for (long long i = N - 1; i >= 2 && (int)out.top10_desc.size() < 10; --i) {
        if (is_prime[(size_t)i]) out.top10_desc.push_back(i);
    }
    return out;
}

// Longitud de bloque en función del progreso (crece con la posición)
// Block length as a function of progress (grows with position)
static inline long long choose_block_len(long long lo, long long end, long long total) {
    const long long MIN_TILE = 1LL << 18;  // 262,144
    const long long MAX_TILE = 1LL << 21;  // 2,097,152
    double f = (total > 0) ? double(lo - 2) / double(total) : 0.0;
    long long len = (long long)std::llround(MIN_TILE + (MAX_TILE - MIN_TILE) * f);
    if (len < MIN_TILE) len = MIN_TILE;
    if (len > MAX_TILE) len = MAX_TILE;
    if (lo + len - 1 > end) len = end - lo + 1;
    return std::max(0LL, len);
}

// Reclama el siguiente segmento [lo, hi] desde un cursor atómico
// Claim next [lo, hi] segment from an atomic cursor
static bool next_segment(std::atomic<long long>& cursor, long long end, long long total,
                         long long& lo, long long& hi) {
    while (true) {
        long long cur = cursor.load(std::memory_order_relaxed);
        if (cur > end) return false;
        long long len = choose_block_len(cur, end, total);
        if (len <= 0) return false;
        long long nx = cur + len;
        if (cursor.compare_exchange_weak(cur, nx, std::memory_order_acq_rel, std::memory_order_relaxed)) {
            lo = cur;
            hi = cur + len - 1;
            return true;
        }
    }
}

// Criba segmentada de [lo, hi] usando los primos base
// Segmented sieve over [lo, hi] using base primes
static void sieve_segment(long long lo, long long hi,
                          const std::vector<int>& base,
                          unsigned long long& count_out,
                          std::vector<long long>& tail_out) {
    const size_t L = (size_t)(hi - lo + 1);
    std::vector<unsigned char> seg(L, 1);

    if (lo == 0) seg[0] = 0;
    if (lo <= 1 && 1 <= hi) seg[(size_t)(1 - lo)] = 0;

    for (int p : base) {
        long long pp = 1LL * p * p;
        if (pp > hi) break;
        long long start = std::max(pp, ((lo + p - 1) / (long long)p) * (long long)p);
        for (long long j = start; j <= hi; j += p) seg[(size_t)(j - lo)] = 0;
    }

    unsigned long long cnt = 0;
    for (size_t i = 0; i < L; ++i) if (seg[i]) ++cnt;
    count_out = cnt;

    for (long long j = hi; j >= lo && (int)tail_out.size() < 10; --j) {
        if (seg[(size_t)(j - lo)]) tail_out.push_back(j);
    }
}

// Paralelo con “cola” dinámica de segmentos y bloques de tamaño variable
// Parallel with dynamic segment queue and variable block sizes
static PrimeSummary count_primes_parallel_dynamic(long long N, int threads) {
    PrimeSummary out;
    if (N <= 2) return out;

    threads = std::max(1, threads);
    std::vector<int> base = build_base_primes((long long)std::floor(std::sqrt((long double)N)));

    const long long BEGIN = 2;
    const long long END   = N - 1;
    const long long TOTAL = std::max(0LL, END - BEGIN + 1);

    std::atomic<long long> cursor(BEGIN);
    std::vector<std::thread> pool;
    std::vector<unsigned long long> counts(threads, 0);
    std::vector<std::vector<long long>> tails(threads);

    auto worker = [&](int tid) {
        unsigned long long local_count = 0;
        std::vector<long long> local_tails;
        local_tails.reserve(256);

        long long lo, hi;
        while (next_segment(cursor, END, TOTAL, lo, hi)) {
            unsigned long long c = 0;
            std::vector<long long> t;
            sieve_segment(lo, hi, base, c, t);
            local_count += c;
            local_tails.insert(local_tails.end(), t.begin(), t.end());
        }
        counts[tid] = local_count;
        tails[tid]  = std::move(local_tails);
    };

    pool.reserve(threads);
    for (int t = 0; t < threads; ++t) pool.emplace_back(worker, t);
    for (auto& th : pool) th.join();

    for (auto c : counts) out.total += c;

    std::vector<long long> merged;
    for (auto& v : tails) merged.insert(merged.end(), v.begin(), v.end());
    std::sort(merged.begin(), merged.end(), std::greater<long long>());
    for (size_t i = 0; i < merged.size() && i < 10; ++i) out.top10_desc.push_back(merged[i]);

    return out;
}

// Imprime el top 10 de primos en orden descendente
// Prints the top 10 primes in descending order
static void print_top10(const std::vector<long long>& v) {
    std::cout << "Top 10 primos (desc): ";
    if (v.empty()) { std::cout << "(ninguno)\n"; return; }
    for (size_t i = 0; i < v.size(); ++i) {
        if (i) std::cout << ' ';
        std::cout << v[i];
    }
    std::cout << '\n';
}

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    long long N;

    std::cout << "Ingrese N (>= 10000000): ";
    // Prompt for N (>= 10,000,000)
    if (!(std::cin >> N) || N < 2) {
        std::cerr << "Entrada inválida para N.\n";
        // Invalid input for N.
        return 1;
    }

    // Usar todos los hilos disponibles del hardware
    unsigned int hw = std::thread::hardware_concurrency();
    int num_threads = hw ? static_cast<int>(hw) : 1;
    std::cout << "N=" << N << "  hilos=" << num_threads << " (todos los disponibles)\n";

    // Secuencial
    // Sequential
    auto t0 = Clock::now();
    PrimeSummary seq = count_primes_sequential(N);
    auto t1 = Clock::now();
    double ms_seq = std::chrono::duration_cast<Ms>(t1 - t0).count();

    std::cout << "\n[SECUENCIAL]\n";
    std::cout << "Cantidad de primos < N: " << seq.total << "\n";
    print_top10(seq.top10_desc);
    std::cout << std::fixed << std::setprecision(3)
              << "Tiempo secuencial: " << ms_seq/1000.0 << " s (" << ms_seq << " ms)\n";

    // Paralelo
    // Parallel
    auto t2 = Clock::now();
    PrimeSummary par = count_primes_parallel_dynamic(N, num_threads);
    auto t3 = Clock::now();
    double ms_par = std::chrono::duration_cast<Ms>(t3 - t2).count();

    std::cout << "\n[MULTIHILO]\n";
    std::cout << "Cantidad de primos < N: " << par.total << "\n";
    print_top10(par.top10_desc);
    std::cout << std::fixed << std::setprecision(3)
              << "Tiempo multihilo: " << ms_par/1000.0 << " s (" << ms_par << " ms)\n";

    if (seq.total != par.total) {
        std::cout << "ADVERTENCIA: difiere la cantidad (seq=" << seq.total
                  << ", par=" << par.total << ")\n";
        // WARNING: counts differ between sequential and parallel.
    }
    if (ms_par > 0.0) {
        double speedup = ms_seq / ms_par;
        std::cout << "Speedup = " << std::setprecision(2) << std::fixed << speedup << "x\n";
        // Speedup metric.
        // Métrica de aceleración.
    }

    return 0;
}
