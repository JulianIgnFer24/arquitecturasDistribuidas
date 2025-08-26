#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>
#include <algorithm>
#include <cmath>
#include <iomanip>

class PrimeFinder {
private:
    long long int N;
    std::vector<long long int> primes;
    std::mutex primes_mutex;
    std::atomic<long long int> primes_count{0};
    
public:
    PrimeFinder(long long int n) : N(n) {
        if (N < 10000000) {
            std::cout << "Advertencia: N debería ser al menos 10^7 (10,000,000)" << std::endl;
        }
    }
    
    // Función para verificar si un número es primo (optimizada)
    bool is_prime(long long int num) {
        if (num < 2) return false;
        if (num == 2) return true;
        if (num % 2 == 0) return false;
        
        // Solo verificar divisores impares hasta sqrt(num)
        long long int limit = static_cast<long long int>(std::sqrt(num));
        for (long long int i = 3; i <= limit; i += 2) {
            if (num % i == 0) {
                return false;
            }
        }
        return true;
    }
    
    // Implementación usando Criba de Eratóstenes (más eficiente para rangos grandes)
    std::vector<long long int> sieve_of_eratosthenes(long long int limit) {
        std::vector<bool> is_prime_arr(limit, true);
        std::vector<long long int> primes_result;
        
        is_prime_arr[0] = is_prime_arr[1] = false;
        
        for (long long int i = 2; i * i < limit; ++i) {
            if (is_prime_arr[i]) {
                for (long long int j = i * i; j < limit; j += i) {
                    is_prime_arr[j] = false;
                }
            }
        }
        
        for (long long int i = 2; i < limit; ++i) {
            if (is_prime_arr[i]) {
                primes_result.push_back(i);
            }
        }
        
        return primes_result;
    }
    
    // Implementación secuencial
    void find_primes_sequential() {
        std::cout << "\n=== BÚSQUEDA SECUENCIAL DE PRIMOS ===" << std::endl;
        std::cout << "Buscando primos menores a " << N << "..." << std::endl;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        primes.clear();
        primes = sieve_of_eratosthenes(N);
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        std::cout << "Tiempo de ejecución secuencial: " << duration.count() << " ms" << std::endl;
        std::cout << "Cantidad de números primos encontrados: " << primes.size() << std::endl;
        
        show_largest_primes();
    }
    
    // Implementación con multithreading usando segmentación
    void find_primes_multithreaded() {
        std::cout << "\n=== BÚSQUEDA CON MULTITHREADING ===" << std::endl;
        std::cout << "Buscando primos menores a " << N << "..." << std::endl;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        const int num_threads = std::thread::hardware_concurrency();
        std::vector<std::thread> threads;
        std::vector<std::vector<long long int>> thread_primes(num_threads);
        
        // Dividir el rango entre los hilos
        long long int range_per_thread = N / num_threads;
        
        for (int i = 0; i < num_threads; ++i) {
            long long int start = i * range_per_thread;
            long long int end = (i == num_threads - 1) ? N : (i + 1) * range_per_thread;
            
            // Ajustar para evitar duplicados y asegurar que 2 sea incluido
            if (i == 0) {
                start = 2;
            } else {
                start = std::max(start, 2LL);
                // Asegurar que el inicio sea impar si no es 2
                if (start > 2 && start % 2 == 0) {
                    start++;
                }
            }
            
            threads.emplace_back([this, i, start, end, &thread_primes]() {
                find_primes_in_range(start, end, thread_primes[i]);
            });
        }
        
        // Esperar a que todos los hilos terminen
        for (auto& thread : threads) {
            thread.join();
        }
        
        // Combinar resultados de todos los hilos
        primes.clear();
        for (const auto& thread_prime_list : thread_primes) {
            primes.insert(primes.end(), thread_prime_list.begin(), thread_prime_list.end());
        }
        
        // Ordenar los primos (necesario porque cada hilo trabajó en un rango diferente)
        std::sort(primes.begin(), primes.end());
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        std::cout << "Tiempo de ejecución con " << num_threads << " hilos: " << duration.count() << " ms" << std::endl;
        std::cout << "Cantidad de números primos encontrados: " << primes.size() << std::endl;
        
        show_largest_primes();
    }
    
    // Función que ejecuta cada hilo para buscar primos en un rango específico
    void find_primes_in_range(long long int start, long long int end, std::vector<long long int>& local_primes) {
        for (long long int num = start; num < end; ++num) {
            if (is_prime(num)) {
                local_primes.push_back(num);
            }
        }
    }
    
    // Implementación alternativa con criba segmentada (más eficiente)
    void find_primes_segmented_sieve() {
        std::cout << "\n=== BÚSQUEDA CON CRIBA SEGMENTADA (MULTITHREADING) ===" << std::endl;
        std::cout << "Buscando primos menores a " << N << "..." << std::endl;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Primero, encontrar todos los primos hasta sqrt(N) usando criba simple
        long long int limit = static_cast<long long int>(std::sqrt(N)) + 1;
        std::vector<long long int> base_primes = sieve_of_eratosthenes(limit);
        
        const int num_threads = std::thread::hardware_concurrency();
        std::vector<std::thread> threads;
        std::vector<std::vector<long long int>> thread_results(num_threads);
        
        // Tamaño del segmento
        long long int segment_size = std::max(1000000LL, (N - limit) / num_threads);
        
        for (int i = 0; i < num_threads; ++i) {
            long long int start = limit + i * segment_size;
            long long int end = std::min(start + segment_size, N);
            
            if (start >= N) break;
            
            threads.emplace_back([this, start, end, &base_primes, &thread_results, i]() {
                segmented_sieve(start, end, base_primes, thread_results[i]);
            });
        }
        
        // Esperar a todos los hilos
        for (auto& thread : threads) {
            thread.join();
        }
        
        // Combinar resultados
        primes = base_primes; // Incluir primos base
        for (const auto& thread_result : thread_results) {
            primes.insert(primes.end(), thread_result.begin(), thread_result.end());
        }
        
        std::sort(primes.begin(), primes.end());
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        std::cout << "Tiempo de ejecución con criba segmentada (" << num_threads << " hilos): " 
                  << duration.count() << " ms" << std::endl;
        std::cout << "Cantidad de números primos encontrados: " << primes.size() << std::endl;
        
        show_largest_primes();
    }
    
    // Criba segmentada para un rango específico
    void segmented_sieve(long long int start, long long int end, 
                        const std::vector<long long int>& base_primes,
                        std::vector<long long int>& result) {
        
        std::vector<bool> is_prime_seg(end - start, true);
        
        for (long long int prime : base_primes) {
            // Encontrar el primer múltiplo de prime >= start
            long long int first_multiple = ((start + prime - 1) / prime) * prime;
            
            // Marcar múltiplos como no primos
            for (long long int j = first_multiple; j < end; j += prime) {
                is_prime_seg[j - start] = false;
            }
        }
        
        // Recopilar primos del segmento
        for (long long int i = 0; i < end - start; ++i) {
            if (is_prime_seg[i] && (start + i) >= 2) {
                result.push_back(start + i);
            }
        }
    }
    
    // Mostrar los 10 mayores números primos
    void show_largest_primes() {
        std::cout << "\n=== LOS 10 MAYORES NÚMEROS PRIMOS ENCONTRADOS ===" << std::endl;
        
        if (primes.empty()) {
            std::cout << "No se encontraron números primos." << std::endl;
            return;
        }
        
        size_t count = std::min(static_cast<size_t>(10), primes.size());
        
        for (size_t i = 0; i < count; ++i) {
            size_t index = primes.size() - 1 - i;
            std::cout << (i + 1) << ". " << std::setw(12) << primes[index] << std::endl;
        }
    }
    
    // Función de benchmark completa
    void benchmark_comparison() {
        std::cout << "\n=== COMPARACIÓN COMPLETA DE RENDIMIENTO ===" << std::endl;
        
        // Medición secuencial
        auto start_sequential = std::chrono::high_resolution_clock::now();
        auto sequential_primes = sieve_of_eratosthenes(N);
        auto end_sequential = std::chrono::high_resolution_clock::now();
        auto sequential_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_sequential - start_sequential);
        
        // Medición con multithreading (método simple)
        auto start_threaded = std::chrono::high_resolution_clock::now();
        const int num_threads = std::thread::hardware_concurrency();
        std::vector<std::thread> threads;
        std::vector<std::vector<long long int>> thread_primes(num_threads);
        
        long long int range_per_thread = N / num_threads;
        
        for (int i = 0; i < num_threads; ++i) {
            long long int start = i * range_per_thread;
            long long int end = (i == num_threads - 1) ? N : (i + 1) * range_per_thread;
            
            if (i == 0) {
                start = 2;
            } else {
                start = std::max(start, 2LL);
                if (start > 2 && start % 2 == 0) {
                    start++;
                }
            }
            
            threads.emplace_back([this, i, start, end, &thread_primes]() {
                find_primes_in_range(start, end, thread_primes[i]);
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        std::vector<long long int> threaded_primes;
        for (const auto& thread_prime_list : thread_primes) {
            threaded_primes.insert(threaded_primes.end(), thread_prime_list.begin(), thread_prime_list.end());
        }
        std::sort(threaded_primes.begin(), threaded_primes.end());
        
        auto end_threaded = std::chrono::high_resolution_clock::now();
        auto threaded_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_threaded - start_threaded);
        
        // Mostrar resultados
        std::cout << "\n=== RESULTADOS ===" << std::endl;
        std::cout << "Cantidad de números primos encontrados: " << sequential_primes.size() << std::endl;
        
        std::cout << "\nLos 10 mayores números primos:" << std::endl;
        size_t count = std::min(static_cast<size_t>(10), sequential_primes.size());
        for (size_t i = 0; i < count; ++i) {
            size_t index = sequential_primes.size() - 1 - i;
            std::cout << (i + 1) << ". " << std::setw(12) << sequential_primes[index] << std::endl;
        }
        
        // Calcular métricas
        double speedup = static_cast<double>(sequential_duration.count()) / threaded_duration.count();
        double efficiency = speedup / num_threads;
        
        std::cout << "\n=== MÉTRICAS DE RENDIMIENTO ===" << std::endl;
        std::cout << "Tiempo secuencial: " << sequential_duration.count() << " ms" << std::endl;
        std::cout << "Tiempo con " << num_threads << " hilos: " << threaded_duration.count() << " ms" << std::endl;
        std::cout << "Speedup: " << std::fixed << std::setprecision(2) << speedup << "x" << std::endl;
        std::cout << "Eficiencia: " << std::fixed << std::setprecision(2) << (efficiency * 100) << "%" << std::endl;
        
        // Información del sistema
        std::cout << "\n=== INFORMACIÓN DEL SISTEMA ===" << std::endl;
        std::cout << "Núcleos disponibles: " << std::thread::hardware_concurrency() << std::endl;
        std::cout << "Hilos utilizados: " << num_threads << std::endl;
        std::cout << "Rango analizado: 2 a " << (N - 1) << std::endl;
        std::cout << "Números evaluados: " << (N - 2) << std::endl;
        
        // Verificación de corrección
        if (sequential_primes.size() != threaded_primes.size()) {
            std::cout << "\n¡ADVERTENCIA! Las implementaciones produjeron resultados diferentes:" << std::endl;
            std::cout << "Secuencial: " << sequential_primes.size() << " primos" << std::endl;
            std::cout << "Multithreading: " << threaded_primes.size() << " primos" << std::endl;
        } else {
            std::cout << "\n✓ Las implementaciones produjeron resultados consistentes" << std::endl;
        }
    }
    
    // Función para probar diferentes tamaños
    void performance_scaling_test() {
        std::cout << "\n=== TEST DE ESCALABILIDAD ===" << std::endl;
        
        std::vector<long long int> test_sizes = {1000000, 5000000, 10000000, 20000000};
        
        for (long long int test_n : test_sizes) {
            if (test_n > N) continue;
            
            std::cout << "\nTesting N = " << test_n << std::endl;
            
            // Secuencial
            auto start = std::chrono::high_resolution_clock::now();
            auto primes_seq = sieve_of_eratosthenes(test_n);
            auto end = std::chrono::high_resolution_clock::now();
            auto seq_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            
            // Multithreaded
            start = std::chrono::high_resolution_clock::now();
            const int num_threads = std::thread::hardware_concurrency();
            std::vector<std::thread> threads;
            std::vector<std::vector<long long int>> thread_results(num_threads);
            
            long long int range_per_thread = test_n / num_threads;
            for (int i = 0; i < num_threads; ++i) {
                long long int thread_start = i * range_per_thread;
                long long int thread_end = (i == num_threads - 1) ? test_n : (i + 1) * range_per_thread;
                
                if (i == 0) {
                    thread_start = 2;
                } else {
                    thread_start = std::max(thread_start, 2LL);
                    if (thread_start > 2 && thread_start % 2 == 0) {
                        thread_start++;
                    }
                }
                
                threads.emplace_back([this, i, thread_start, thread_end, &thread_results]() {
                    find_primes_in_range(thread_start, thread_end, thread_results[i]);
                });
            }
            
            for (auto& thread : threads) {
                thread.join();
            }
            
            end = std::chrono::high_resolution_clock::now();
            auto mt_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            
            double speedup = static_cast<double>(seq_time.count()) / mt_time.count();
            
            std::cout << "  Primos encontrados: " << primes_seq.size() << std::endl;
            std::cout << "  Tiempo secuencial: " << seq_time.count() << " ms" << std::endl;
            std::cout << "  Tiempo multithreading: " << mt_time.count() << " ms" << std::endl;
            std::cout << "  Speedup: " << std::fixed << std::setprecision(2) << speedup << "x" << std::endl;
        }
    }
};

int main() {
    std::cout << "=== PROGRAMA DE BÚSQUEDA DE NÚMEROS PRIMOS ===" << std::endl;
    std::cout << "Ejercicio N°4" << std::endl;
    
    long long int N;
    std::cout << "\nIngrese el valor de N (debe ser al menos 10^7 = 10,000,000): ";
    std::cin >> N;
    
    if (N < 2) {
        std::cout << "Error: N debe ser mayor que 1" << std::endl;
        return 1;
    }
    
    try {
        PrimeFinder finder(N);
        
        std::cout << "\nSeleccione el modo de ejecución:" << std::endl;
        std::cout << "1. Secuencial" << std::endl;
        std::cout << "2. Multithreading" << std::endl;
        std::cout << "3. Criba segmentada (recomendado para N grandes)" << std::endl;
        std::cout << "4. Benchmark completo" << std::endl;
        std::cout << "5. Test de escalabilidad" << std::endl;
        std::cout << "Opción: ";
        
        int option;
        std::cin >> option;
        
        switch (option) {
            case 1:
                finder.find_primes_sequential();
                break;
            case 2:
                finder.find_primes_multithreaded();
                break;
            case 3:
                finder.find_primes_segmented_sieve();
                break;
            case 4:
                finder.benchmark_comparison();
                break;
            case 5:
                finder.performance_scaling_test();
                break;
            default:
                std::cout << "Opción inválida. Ejecutando benchmark completo..." << std::endl;
                finder.benchmark_comparison();
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

/*
Instrucciones de compilación y ejecución:

1. Compilar:
   g++ -std=c++11 -O3 -pthread -o prime_finder prime_finder.cpp

2. Ejecutar:
   ./prime_finder

3. Valores recomendados para testing:
   - N = 10,000,000 (10^7) - mínimo requerido
   - N = 50,000,000 (5×10^7) - para ver diferencias claras
   - N = 100,000,000 (10^8) - para pruebas intensivas

4. Para monitoreo de CPU:
   - Abrir monitor del sistema antes de ejecutar
   - Observar diferencias entre ejecución secuencial y multithreading
   - Notar distribución de carga entre núcleos

Optimizaciones implementadas:
- Criba de Eratóstenes para eficiencia máxima
- Criba segmentada para rangos muy grandes
- División inteligente del trabajo entre hilos
- Verificación solo de números impares (excepto 2)
- Límite de búsqueda hasta sqrt(n) para verificación de primalidad

Notas de rendimiento:
- La criba segmentada es más eficiente para N > 10^7
- El speedup depende del número de núcleos disponibles
- La eficiencia puede variar según el tamaño del problema
*/