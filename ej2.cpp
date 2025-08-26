#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <mutex>
#include <algorithm>

class PatternSearcher {
private:
    std::string text;
    std::vector<std::string> patterns;
    std::mutex output_mutex;
    
public:
    // Constructor - carga los archivos
    PatternSearcher() {
        load_text_file("texto_ej2.txt");
        load_patterns_file("patrones.txt");
    }
    
    // Cargar archivo de texto (200MB)
    bool load_text_file(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        
        if (!file.is_open()) {
            std::cerr << "Error: No se pudo abrir el archivo " << filename << std::endl;
            return false;
        }
        
        // Obtener tamaño del archivo
        file.seekg(0, std::ios::end);
        size_t file_size = file.tellg();
        file.seekg(0, std::ios::beg);
        
        std::cout << "Cargando archivo de texto (" << file_size / (1024 * 1024) << " MB)..." << std::endl;
        
        // Reservar memoria y leer todo el archivo
        text.reserve(file_size);
        text.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
        
        file.close();
        
        std::cout << "Archivo cargado exitosamente. Tamaño: " << text.length() << " caracteres" << std::endl;
        return true;
    }
    
    // Cargar patrones desde archivo
    bool load_patterns_file(const std::string& filename) {
        std::ifstream file(filename);
        
        if (!file.is_open()) {
            std::cerr << "Error: No se pudo abrir el archivo " << filename << std::endl;
            return false;
        }
        
        std::string pattern;
        while (std::getline(file, pattern)) {
            if (!pattern.empty()) {
                patterns.push_back(pattern);
            }
        }
        
        file.close();
        
        std::cout << "Cargados " << patterns.size() << " patrones" << std::endl;
        return true;
    }
    
    // Función para contar ocurrencias de un patrón en el texto
    size_t count_pattern_occurrences(const std::string& pattern) const {
        if (pattern.empty() || text.empty()) {
            return 0;
        }
        
        size_t count = 0;
        size_t pos = 0;
        
        // Búsqueda usando std::string::find
        while ((pos = text.find(pattern, pos)) != std::string::npos) {
            count++;
            pos += 1; // Permitir solapamiento
        }
        
        return count;
    }
    
    // Versión optimizada usando algoritmo KMP (Knuth-Morris-Pratt)
    size_t count_pattern_occurrences_kmp(const std::string& pattern) const {
        if (pattern.empty() || text.empty()) {
            return 0;
        }
        
        // Construir tabla de fallos para KMP
        std::vector<int> failure_table(pattern.length(), 0);
        build_failure_table(pattern, failure_table);
        
        size_t count = 0;
        size_t i = 0; // índice para texto
        size_t j = 0; // índice para patrón
        
        while (i < text.length()) {
            if (text[i] == pattern[j]) {
                i++;
                j++;
                
                if (j == pattern.length()) {
                    count++;
                    j = failure_table[j - 1];
                }
            } else if (j > 0) {
                j = failure_table[j - 1];
            } else {
                i++;
            }
        }
        
        return count;
    }
    
private:
    // Construir tabla de fallos para algoritmo KMP
    void build_failure_table(const std::string& pattern, std::vector<int>& table) const {
        int j = 0;
        for (size_t i = 1; i < pattern.length(); i++) {
            while (j > 0 && pattern[i] != pattern[j]) {
                j = table[j - 1];
            }
            if (pattern[i] == pattern[j]) {
                j++;
            }
            table[i] = j;
        }
    }
    
public:
    // Implementación secuencial
    void search_patterns_sequential() {
        std::cout << "\n=== BÚSQUEDA SECUENCIAL ===" << std::endl;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        for (size_t i = 0; i < patterns.size(); i++) {
            size_t count = count_pattern_occurrences_kmp(patterns[i]);
            std::cout << "el patron " << i << " aparece " << count << " veces" << std::endl;
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        std::cout << "\nTiempo de ejecución secuencial: " << duration.count() << " ms" << std::endl;
    }
    
    // Función que ejecutará cada hilo
    void search_pattern_thread(size_t pattern_index, std::vector<size_t>& results) {
        if (pattern_index < patterns.size()) {
            results[pattern_index] = count_pattern_occurrences_kmp(patterns[pattern_index]);
        }
    }
    
    // Implementación con multithreading (32 hilos)
    void search_patterns_multithreaded() {
        std::cout << "\n=== BÚSQUEDA CON 32 HILOS ===" << std::endl;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        std::vector<std::thread> threads;
        std::vector<size_t> results(patterns.size(), 0);
        
        // Crear 32 hilos (uno por patrón, asumiendo que hay 32 patrones)
        for (size_t i = 0; i < patterns.size() && i < 32; i++) {
            threads.emplace_back([this, i, &results]() {
                search_pattern_thread(i, results);
            });
        }
        
        // Esperar a que todos los hilos terminen
        for (auto& thread : threads) {
            thread.join();
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        // Mostrar resultados
        for (size_t i = 0; i < patterns.size(); i++) {
            std::cout << "el patron " << i << " aparece " << results[i] << " veces" << std::endl;
        }
        
        std::cout << "\nTiempo de ejecución con hilos: " << duration.count() << " ms" << std::endl;
    }
    
    // Implementación con pool de hilos más eficiente
    void search_patterns_thread_pool() {
        std::cout << "\n=== BÚSQUEDA CON POOL DE HILOS ===" << std::endl;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        const size_t num_threads = std::min(static_cast<size_t>(32), patterns.size());
        std::vector<std::thread> threads;
        std::vector<size_t> results(patterns.size(), 0);
        std::atomic<size_t> pattern_index{0};
        
        // Crear hilos que trabajen en un pool
        for (size_t i = 0; i < num_threads; i++) {
            threads.emplace_back([this, &results, &pattern_index]() {
                size_t current_pattern;
                while ((current_pattern = pattern_index.fetch_add(1)) < patterns.size()) {
                    results[current_pattern] = count_pattern_occurrences_kmp(patterns[current_pattern]);
                }
            });
        }
        
        // Esperar a que todos los hilos terminen
        for (auto& thread : threads) {
            thread.join();
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        // Mostrar resultados
        for (size_t i = 0; i < patterns.size(); i++) {
            std::cout << "el patron " << i << " aparece " << results[i] << " veces" << std::endl;
        }
        
        std::cout << "\nTiempo de ejecución con pool de hilos: " << duration.count() << " ms" << std::endl;
    }
    
    // Función para calcular y mostrar el speedup
    void benchmark_comparison() {
        std::cout << "\n=== COMPARACIÓN DE RENDIMIENTO ===" << std::endl;
        
        // Medir tiempo secuencial
        auto start_sequential = std::chrono::high_resolution_clock::now();
        std::vector<size_t> sequential_results(patterns.size());
        for (size_t i = 0; i < patterns.size(); i++) {
            sequential_results[i] = count_pattern_occurrences_kmp(patterns[i]);
        }
        auto end_sequential = std::chrono::high_resolution_clock::now();
        auto sequential_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_sequential - start_sequential);
        
        // Medir tiempo con hilos
        auto start_threaded = std::chrono::high_resolution_clock::now();
        const size_t num_threads = std::min(static_cast<size_t>(32), patterns.size());
        std::vector<std::thread> threads;
        std::vector<size_t> threaded_results(patterns.size(), 0);
        std::atomic<size_t> pattern_index{0};
        
        for (size_t i = 0; i < num_threads; i++) {
            threads.emplace_back([this, &threaded_results, &pattern_index]() {
                size_t current_pattern;
                while ((current_pattern = pattern_index.fetch_add(1)) < patterns.size()) {
                    threaded_results[current_pattern] = count_pattern_occurrences_kmp(patterns[current_pattern]);
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        auto end_threaded = std::chrono::high_resolution_clock::now();
        auto threaded_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_threaded - start_threaded);
        
        // Mostrar resultados
        std::cout << "Resultados:" << std::endl;
        for (size_t i = 0; i < patterns.size(); i++) {
            std::cout << "el patron " << i << " aparece " << sequential_results[i] << " veces" << std::endl;
        }
        
        // Calcular speedup
        double speedup = static_cast<double>(sequential_duration.count()) / threaded_duration.count();
        double efficiency = speedup / num_threads;
        
        std::cout << "\n=== MÉTRICAS DE RENDIMIENTO ===" << std::endl;
        std::cout << "Tiempo secuencial: " << sequential_duration.count() << " ms" << std::endl;
        std::cout << "Tiempo con " << num_threads << " hilos: " << threaded_duration.count() << " ms" << std::endl;
        std::cout << "Speedup: " << speedup << "x" << std::endl;
        std::cout << "Eficiencia: " << (efficiency * 100) << "%" << std::endl;
        
        // Información del sistema
        std::cout << "\n=== INFORMACIÓN DEL SISTEMA ===" << std::endl;
        std::cout << "Núcleos disponibles: " << std::thread::hardware_concurrency() << std::endl;
        std::cout << "Hilos utilizados: " << num_threads << std::endl;
        std::cout << "Tamaño del texto: " << text.length() << " caracteres" << std::endl;
        std::cout << "Número de patrones: " << patterns.size() << std::endl;
    }
    
    // Función para mostrar información de los patrones
    void show_pattern_info() {
        std::cout << "\n=== INFORMACIÓN DE PATRONES ===" << std::endl;
        for (size_t i = 0; i < std::min(patterns.size(), static_cast<size_t>(10)); i++) {
            std::cout << "Patrón " << i << ": \"" << patterns[i] << "\" (longitud: " << patterns[i].length() << ")" << std::endl;
        }
        if (patterns.size() > 10) {
            std::cout << "... y " << (patterns.size() - 10) << " patrones más" << std::endl;
        }
    }
};

int main() {
    std::cout << "=== PROGRAMA DE BÚSQUEDA DE PATRONES ===" << std::endl;
    std::cout << "Trabajo Práctico N°1" << std::endl;
    
    try {
        PatternSearcher searcher;
        
        // Mostrar información de los patrones
        searcher.show_pattern_info();
        
        // Realizar benchmark completo
        searcher.benchmark_comparison();
        
        std::cout << "\n=== INSTRUCCIONES PARA MONITOREO ===" << std::endl;
        std::cout << "Para observar el uso de CPU por núcleo:" << std::endl;
        std::cout << "- Windows: Usar el Administrador de tareas (Ctrl+Shift+Esc)" << std::endl;
        std::cout << "- Linux: usar 'htop' o 'top' en terminal" << std::endl;
        std::cout << "- macOS: usar 'Activity Monitor' o 'htop'" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

/*
Instrucciones de compilación y ejecución:

1. Compilar:
   g++ -std=c++11 -O3 -pthread -o pattern_search pattern_search.cpp

2. Ejecutar:
   ./pattern_search

3. Para monitoreo de CPU:
   - Ejecutar el programa en una terminal
   - Abrir otra terminal y ejecutar 'htop' o 'top'
   - Observar el uso de CPU durante la ejecución

4. Archivos necesarios:
   - texto_ej2.txt (archivo de 200MB)
   - patrones.txt (archivo con 32 patrones, uno por línea)

Notas de optimización:
- Usa algoritmo KMP para búsqueda eficiente
- Implementa pool de hilos para mejor balanceo de carga
- Minimiza sincronización entre hilos
- Usa atomic para contador de patrones
- Optimizado con flags de compilación -O3
*/