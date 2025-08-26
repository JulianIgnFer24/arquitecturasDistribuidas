#include <iostream>
#include <vector>
#include <thread>
#include <sys/time.h>  // para gettimeofday
#include <iomanip>

using namespace std;

using Matrix = vector<vector<float>>;

// Inicializar matriz con un valor fijo
Matrix initMatrix(int N, float value) {
    return Matrix(N, vector<float>(N, value));
}

// Multiplicación secuencial
Matrix multiplySequential(const Matrix &A, const Matrix &B, int N) {
    Matrix C(N, vector<float>(N, 0.0f));
    for(int i = 0; i < N; i++) {
        for(int j = 0; j < N; j++) {
            for(int k = 0; k < N; k++) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }
    return C;
}

// Multiplicación de un bloque de filas (para los hilos)
void multiplyBlock(const Matrix &A, const Matrix &B, Matrix &C, int startRow, int endRow, int N) {
    for(int i = startRow; i < endRow; i++) {
        for(int j = 0; j < N; j++) {
            for(int k = 0; k < N; k++) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }
}

// Multiplicación paralela
Matrix multiplyParallel(const Matrix &A, const Matrix &B, int N, int numThreads) {
    Matrix C(N, vector<float>(N, 0.0f));
    vector<thread> threads;
    int blockSize = N / numThreads;
    int startRow = 0;

    for(int t = 0; t < numThreads; t++) {
        int endRow = (t == numThreads - 1) ? N : startRow + blockSize;
        threads.emplace_back(multiplyBlock, cref(A), cref(B), ref(C), startRow, endRow, N);
        startRow = endRow;
    }

    for(auto &th : threads) {
        th.join();
    }

    return C;
}

// Suma todos los elementos
double sumMatrix(const Matrix &M, int N) {
    double total = 0.0;
    for(int i = 0; i < N; i++)
        for(int j = 0; j < N; j++)
            total += M[i][j];
    return total;
}

// Imprimir esquinas
void printCorners(const Matrix &M, int N, const string &name) {
    cout << "Esquinas de " << name << ":\n";
    cout << fixed << setprecision(4);
    cout << M[0][0] << " ... " << M[0][N-1] << "\n";
    cout << "...\n";
    cout << M[N-1][0] << " ... " << M[N-1][N-1] << "\n\n";
}

int main() {
    int N, numThreads;
    cout << "Ingrese el tamaño N de la matriz: ";
    cin >> N;
    cout << "Ingrese la cantidad de hilos: ";
    cin >> numThreads;

    // Inicializar matrices con los valores del enunciado
    Matrix A = initMatrix(N, 0.1f);
    Matrix B = initMatrix(N, 0.2f);

    // ---------------- SECUENCIAL ----------------
    timeval t1, t2;
    gettimeofday(&t1, NULL);
    Matrix C1 = multiplySequential(A, B, N);
    gettimeofday(&t2, NULL);

    double timeSeq = double(t2.tv_sec - t1.tv_sec) +
                     double(t2.tv_usec - t1.tv_usec) / 1000000.0;

    double sumSeq = sumMatrix(C1, N);

    cout << "\n==== Resultado SECUENCIAL ====\n";
    printCorners(C1, N, "Matriz C (secuencial)");
    cout << "Sumatoria: " << sumSeq << "\n";
    cout << "Tiempo de ejecución: " << timeSeq << " segundos\n\n";

    // ---------------- PARALELO ----------------
    gettimeofday(&t1, NULL);
    Matrix C2 = multiplyParallel(A, B, N, numThreads);
    gettimeofday(&t2, NULL);

    double timePar = double(t2.tv_sec - t1.tv_sec) +
                     double(t2.tv_usec - t1.tv_usec) / 1000000.0;

    double sumPar = sumMatrix(C2, N);

    cout << "==== Resultado PARALELO ====\n";
    printCorners(C2, N, "Matriz C (paralela)");
    cout << "Sumatoria: " << sumPar << "\n";
    cout << "Tiempo de ejecución: " << timePar << " segundos\n\n";

    // ---------------- SPEEDUP ----------------
    cout << "==== SPEEDUP ====\n";
    cout << "Speedup = TiempoSecuencial / TiempoParalelo = "
         << timeSeq / timePar << "\n";

    return 0;
}
