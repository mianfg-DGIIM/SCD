// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Práctica 3. Implementación de algoritmos distribuidos con MPI
//
// Archivo: prodcons.cpp
// Implementación del problema del productor-consumidor con
// un proceso intermedio que recibe mensajes síncronos de forma alterna.
// (versión con un único productor y un único consumidor)
//
// Historial:
// Actualizado a C++11 en Septiembre de 2017
// -----------------------------------------------------------------------------

#include <mpi.h>   // includes de MPI
#include <chrono>  // duraciones (duration), unidades de tiempo
#include <iostream>
#include <random>  // dispositivos, generadores y distribuciones aleatorias
#include <thread>  // this_thread::sleep_for

using namespace std;
using namespace std::this_thread;
using namespace std::chrono;

// ---------------------------------------------------------------------
// constantes que determinan la asignación de identificadores a roles:
const int id_productor = 0,     // identificador del proceso productor
    id_buffer = 1,              // identificador del proceso buffer
    id_consumidor = 2,          // identificador del proceso consumidor
    num_procesos_esperado = 3,  // número total de procesos esperado
    num_items = 20;             // numero de items producidos o consumidos

//**********************************************************************
// plantilla de función para generar un entero aleatorio uniformemente
// distribuido entre dos valores enteros, ambos incluidos
// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
//----------------------------------------------------------------------

template <int min, int max>
int aleatorio() {
    static default_random_engine generador((random_device())());
    static uniform_int_distribution<int> distribucion_uniforme(min, max);
    return distribucion_uniforme(generador);
}
// ---------------------------------------------------------------------
// produce los numeros en secuencia (1,2,3,....)

int producir() {
    static int contador = 0;
    sleep_for(milliseconds(aleatorio<10, 200>()));
    contador++;
    cout << "Productor ha producido valor " << contador << flush << endl << flush;
    return contador;
}
// ---------------------------------------------------------------------

void funcion_productor() {
    for ( int i = 0; i < num_items; i++ ) {
        int dato = producir();
        cout << "Productor enviará valor " << dato << flush << endl;
        MPI_Ssend(&dato, 1, MPI_INT, id_buffer, 0, MPI_COMM_WORLD);
    }
}
// ---------------------------------------------------------------------

void consumir(int valor_cons) {
    // espera bloqueada
    sleep_for(milliseconds(aleatorio<10, 200>()));
    cout << "Consumidor ha consumido valor " << valor_cons << flush << endl << flush;
}
// ---------------------------------------------------------------------

void funcion_consumidor() {
    int peticion, valor;
    MPI_Status estado;
    for ( int i = 0; i < num_items; i++ ) {
        // enviamos peticion para recibir valor
        cout << "Consumidor envía petición para recibir valor de buffer" << flush << endl;
        MPI_Ssend(&peticion, 1, MPI_INT, id_buffer, 0, MPI_COMM_WORLD);
        // esperamos a recibir el valor
        MPI_Recv(&valor, 1, MPI_INT, id_buffer, 0, MPI_COMM_WORLD, &estado);
        cout << "Consumidor ha recibido el valor: " << valor << flush << endl;
        consumir(valor);
    }
}
// ---------------------------------------------------------------------

void funcion_buffer() {
    int valor, peticion;
    MPI_Status estado;
    for ( int i = 0; i < num_items; i++ ) {
        MPI_Recv(&valor, 1, MPI_INT, id_productor, 0, MPI_COMM_WORLD, &estado);
        cout << "Buffer recibe valor del productor: " << valor << flush << endl;

        MPI_Recv(&peticion, 1, MPI_INT, id_consumidor, 0, MPI_COMM_WORLD, &estado);
        cout << "Recibida petición de consumidor, enviando dato" << flush << endl;
        MPI_Ssend(&valor, 1, MPI_INT, id_consumidor, 0, MPI_COMM_WORLD); // por qué synchronized?
    }
    
}

// ---------------------------------------------------------------------

int main(int argc, char *argv[]) {
    int id_propio, num_procesos_actual;  // ident. propio, núm. de procesos

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &id_propio);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procesos_actual);

    if (num_procesos_esperado == num_procesos_actual) {
        if (id_propio == id_productor)    // si mi ident. es el del productor
            funcion_productor();          //    ejecutar función del productor
        else if (id_propio == id_buffer)  // si mi ident. es el del buffer
            funcion_buffer();             //    ejecutar función buffer
        else                       // en otro caso, mi ident es consumidor
            funcion_consumidor();  //    ejecutar función consumidor
    } else if (id_propio == 0)     // si hay error, el proceso 0 informa
        cerr << "error: número de procesos distinto del esperado." << flush << endl;

    MPI_Finalize();
    return 0;
}
// ---------------------------------------------------------------------
