// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Práctica 3. Implementación de algoritmos distribuidos con MPI
//
// Archivo: filosofos-plantilla.cpp
// Implementación del problema de los filósofos (sin camarero).
// Plantilla para completar.
//
// Historial:
// Actualizado a C++11 en Septiembre de 2017
// -----------------------------------------------------------------------------


/*
Situación de interbloqueo:

Filósofo 4 solicita ten. izq.5
Filósofo 6 solicita ten. izq.7
Filósofo 2 solicita ten. izq.3
Filósofo 4 solicita ten. der.3
Filósofo 6 solicita ten. der.5
Filósofo 0 solicita ten. izq.1
Filósofo 0 solicita ten. der.9
Ten. 1 ha sido cogido por filo. 0
Ten. 1 ha sido liberado por filo. 0
Ten. 5 ha sido cogido por filo. 4
Ten. 5 ha sido liberado por filo. 4
Ten. 7 ha sido cogido por filo. 6
Ten. 7 ha sido liberado por filo. 6
Filósofo 8 solicita ten. izq.9
Filósofo 8 solicita ten. der.7
Ten. 9 ha sido cogido por filo. 8
Ten. 9 ha sido liberado por filo. 8
Filósofo 2 solicita ten. der.1
Ten. 3 ha sido cogido por filo. 2
Ten. 3 ha sido liberado por filo. 2


--

Filósofo 6 solicita ten. izq.7
Filósofo 0 solicita ten. izq.1
Filósofo 2 solicita ten. izq.3
Filósofo 2 solicita ten. der.1
Filósofo 4 solicita ten. izq.5
Filósofo 8 solicita ten. izq.9
Filósofo 8 solicita ten. der.7
Ten. 3 ha sido cogido por filo. 2
Ten. 3 ha sido liberado por filo. 2
Ten. 9 ha sido cogido por filo. 8
Ten. 9 ha sido liberado por filo. 8
Filósofo 6 solicita ten. der.5
Ten. 7 ha sido cogido por filo. 6
Ten. 7 ha sido liberado por filo. 6
Ten. 5 ha sido cogido por filo. 4
Ten. 5 ha sido liberado por filo. 4
Filósofo 4 solicita ten. der.3
Filósofo 0 solicita ten. der.9
Ten. 1 ha sido cogido por filo. 0
Ten. 1 ha sido liberado por filo. 0
*/

#include <mpi.h>
#include <chrono>  // duraciones (duration), unidades de tiempo
#include <iostream>
#include <random>  // dispositivos, generadores y distribuciones aleatorias
#include <thread>  // this_thread::sleep_for

using namespace std;
using namespace std::this_thread;
using namespace std::chrono;


const int
    num_filosofos = 5,
    num_procesos = 2 * num_filosofos;

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

void funcion_filosofos(int id) {
    int id_ten_izq = (id + 1) % num_procesos,  // id. tenedor izq.
        id_ten_der =
            (id + num_procesos - 1) % num_procesos,  // id. tenedor der.
        peticion;

    while (true) {
        // hacemos que uno de los filósofos solicite los tenedores en el orden contrario
        if ( id == 0 ) {
            cout << "Filósofo " << id << " solicita ten. der." << id_ten_der
                << endl << flush;
            // ... solicitar tenedor derecho (completar)
            MPI_Ssend(&peticion, 1, MPI_INT, id_ten_der, 0, MPI_COMM_WORLD);

            cout << "Filósofo " << id << " solicita ten. izq." << id_ten_izq
                << endl << flush;
            // ... solicitar tenedor izquierdo (completar)
            MPI_Ssend(&peticion, 1, MPI_INT, id_ten_izq, 0, MPI_COMM_WORLD);
        } else {
            cout << "Filósofo " << id << " solicita ten. izq." << id_ten_izq
                << endl << flush;
            // ... solicitar tenedor izquierdo (completar)
            MPI_Ssend(&peticion, 1, MPI_INT, id_ten_izq, 0, MPI_COMM_WORLD);

            cout << "Filósofo " << id << " solicita ten. der." << id_ten_der
                << endl << flush;
            // ... solicitar tenedor derecho (completar)
            MPI_Ssend(&peticion, 1, MPI_INT, id_ten_der, 0, MPI_COMM_WORLD);
        }

        cout << "Filósofo " << id << " comienza a comer" << endl << flush;
        sleep_for(milliseconds(aleatorio<10, 100>()));

        cout << "Filósofo " << id << " suelta ten. izq. " << id_ten_izq << endl << flush;
        // ... soltar el tenedor izquierdo (completar)
        MPI_Ssend(&peticion, 1, MPI_INT, id_ten_izq, 0, MPI_COMM_WORLD);

        cout << "Filósofo " << id << " suelta ten. der. " << id_ten_der << endl << flush;
        // ... soltar el tenedor derecho (completar)
        MPI_Ssend(&peticion, 1, MPI_INT, id_ten_der, 0, MPI_COMM_WORLD);

        cout << "Filosofo " << id << " comienza a pensar" << endl << flush;
        sleep_for(milliseconds(aleatorio<10, 100>()));
    }
}
// ---------------------------------------------------------------------

void funcion_tenedores(int id) {
    int valor, id_filosofo;  // valor recibido, identificador del filósofo
    MPI_Status estado;       // metadatos de las dos recepciones

    while (true) {
        // ...... recibir petición de cualquier filósofo (completar)
        MPI_Recv(&valor, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &estado);
        // ...... guardar en 'id_filosofo' el id. del emisor (completar)
        id_filosofo = estado.MPI_SOURCE;
        cout << "Ten. " << id << " ha sido cogido por filo. " << id_filosofo
             << endl << flush;

        // ...... recibir liberación de filósofo 'id_filosofo' (completar)
        MPI_Recv(&valor, 1, MPI_INT, id_filosofo, 0, MPI_COMM_WORLD, &estado);
        cout << "Ten. " << id << " ha sido liberado por filo. " << id_filosofo
             << endl << flush;
    }
}
// ---------------------------------------------------------------------

int main(int argc, char** argv) {
    int id_propio, num_procesos_actual;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &id_propio);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procesos_actual);

    if (num_procesos == num_procesos_actual) {
        // ejecutar la función correspondiente a 'id_propio'
        if (id_propio % 2 == 0)            // si es par
            funcion_filosofos(id_propio);  //   es un filósofo
        else                               // si es impar
            funcion_tenedores(id_propio);  //   es un tenedor
    } else {
        if (id_propio == 0)  // solo el primero escribe error, indep. del rol
        {
            cout << "el número de procesos esperados es:    " << num_procesos
                 << endl << flush
                 << "el número de procesos en ejecución es: "
                 << num_procesos_actual << endl << flush
                 << "(programa abortado)" << endl << flush;
        }
    }

    MPI_Finalize();
    return 0;
}

// ---------------------------------------------------------------------
