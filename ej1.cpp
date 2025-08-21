#include <iostream>
#include <cmath>
#include <thread>
#include <vector>
#include <chrono>

using namespace std;

// Función que calcula una parte de la serie de Taylor
void calcular_parcial(long double y, long long inicio, long long fin, long double &resultado) {
    long double suma = 0.0;
    for (long long k = inicio; k < fin; k++) {
        long long n = 2 * k + 1; // índices impares
        long double termino = pow(y, n) / n;
        suma += termino;
    }
    resultado = suma;
}

// Versión secuencial
long double ln_secuencial(long double x, long long terminos) {
    long double y = (x - 1) / (x + 1);
    long double suma = 0.0;
    for (long long k = 0; k < terminos; k++) {
        long long n = 2 * k + 1;
        suma += pow(y, n) / n;
    }
    return 2.0 * suma;
}

// Versión paralela
long double ln_paralelo(long double x, long long terminos, int num_hilos) {
    long double y = (x - 1) / (x + 1);

    vector<thread> hilos;
    vector<long double> resultados(num_hilos, 0.0);

    long long bloque = terminos / num_hilos;

    for (int i = 0; i < num_hilos; i++) {
        long long inicio = i * bloque;
        long long fin = (i == num_hilos - 1) ? terminos : (i + 1) * bloque;
        hilos.push_back(thread(calcular_parcial, y, inicio, fin, ref(resultados[i])));
    }

    for (auto &h : hilos) {
        h.join();
    }

    long double suma_total = 0.0;
    for (auto r : resultados) suma_total += r;

    return 2.0 * suma_total;
}

int main() {
    long double x;
    int num_hilos;

    cout << "Ingrese el valor de x (>1.5e6): ";
    cin >> x;
    cout << "Ingrese el numero de hilos: ";
    cin >> num_hilos;

    const long long terminos = 10000000;

    // Secuencial
    auto inicio1 = chrono::high_resolution_clock::now();
    long double res1 = ln_secuencial(x, terminos);
    auto fin1 = chrono::high_resolution_clock::now();
    chrono::duration<double> tiempo1 = fin1 - inicio1;

    cout.precision(15);
    cout << "\n[Secuencial] ln(" << x << ") = " << res1 << endl;
    cout << "Tiempo: " << tiempo1.count() << " segundos" << endl;

    // Paralelo
    auto inicio2 = chrono::high_resolution_clock::now();
    long double res2 = ln_paralelo(x, terminos, num_hilos);
    auto fin2 = chrono::high_resolution_clock::now();
    chrono::duration<double> tiempo2 = fin2 - inicio2;

    cout << "\n[Paralelo] ln(" << x << ") = " << res2 << endl;
    cout << "Tiempo: " << tiempo2.count() << " segundos" << endl;

    // Speedup
    cout << "\nSpeedup = " << tiempo1.count() / tiempo2.count() << endl;

    return 0;
}
