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

#include <mpi.h>
#include <chrono>  // duraciones (duration), unidades de tiempo
#include <iostream>
#include <random>  // dispositivos, generadores y distribuciones aleatorias
#include <thread>  // this_thread::sleep_for

using namespace std;
using namespace std::this_thread;
using namespace std::chrono;

#define TAG_SENTARSE   1
#define TAG_LEVANTARSE 2


const int
    num_filosofos = 5,
    num_procesos = 2 * num_filosofos + 1,
    num_fc = 2*num_filosofos,   // número de filósofos y camareros
    id_camarero = 2*num_filosofos;

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
    int id_ten_izq = (id + 1) % num_fc,  // id. tenedor izq.
        id_ten_der =
            (id + num_fc - 1) % num_fc,  // id. tenedor der.
        peticion;

    while (true) {
        cout << "Filósofo " << id << " solicita sentarse en mesa" << endl;
        MPI_Ssend(&peticion, 1, MPI_INT, id_camarero, TAG_SENTARSE, MPI_COMM_WORLD);

        cout << "Filósofo " << id << " solicita ten. izq." << id_ten_izq
             << endl;
        // ... solicitar tenedor izquierdo (completar)
        MPI_Ssend(&peticion, 1, MPI_INT, id_ten_izq, 0, MPI_COMM_WORLD);

        cout << "Filósofo " << id << " solicita ten. der." << id_ten_der
             << endl;
        // ... solicitar tenedor derecho (completar)
        MPI_Ssend(&peticion, 1, MPI_INT, id_ten_der, 0, MPI_COMM_WORLD);

        cout << "Filósofo " << id << " comienza a comer" << endl;
        sleep_for(milliseconds(aleatorio<10, 100>()));

        cout << "Filósofo " << id << " suelta ten. izq. " << id_ten_izq << endl;
        // ... soltar el tenedor izquierdo (completar)
        MPI_Ssend(&peticion, 1, MPI_INT, id_ten_izq, 0, MPI_COMM_WORLD);

        cout << "Filósofo " << id << " suelta ten. der. " << id_ten_der << endl;
        // ... soltar el tenedor derecho (completar)
        MPI_Ssend(&peticion, 1, MPI_INT, id_ten_der, 0, MPI_COMM_WORLD);

        cout << "Filósofo " << id << " solicita levantarse de la mesa" << endl;
        MPI_Ssend(&peticion, 1, MPI_INT, id_camarero, TAG_LEVANTARSE, MPI_COMM_WORLD);

        cout << "Filosofo " << id << " comienza a pensar" << endl;
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
             << endl;

        // ...... recibir liberación de filósofo 'id_filosofo' (completar)
        MPI_Recv(&valor, 1, MPI_INT, id_filosofo, 0, MPI_COMM_WORLD, &estado);
        cout << "Ten. " << id << " ha sido liberado por filo. " << id_filosofo
             << endl;
    }
}
// ---------------------------------------------------------------------

void funcion_camarero() {
    int valor, id_filosofo, num_sentados = 0, etiq_aceptable;
    MPI_Status estado;

    while (true) {
        // determinar si atiende a peticiones de sentarse y levantarse o sólo de levantarse
        if ( num_sentados < num_filosofos - 1 ) {
            etiq_aceptable = MPI_ANY_TAG;
            //cout << ">>>>>>>>>Camarero espera a cualquiera" << endl;
        }else{
            etiq_aceptable = TAG_LEVANTARSE;
            //cout << ">>>>>>>>>Camarero espera a quien quiera levantarse" << endl;
        }
        
        // pedir una petición
        MPI_Recv(&valor, 1, MPI_INT, MPI_ANY_SOURCE, etiq_aceptable, MPI_COMM_WORLD, &estado);
        id_filosofo = estado.MPI_SOURCE;

        // procesar mensaje
        switch (estado.MPI_TAG) {
            case TAG_SENTARSE:
                cout << "--[S]-- Filósofo " << id_filosofo << " se sienta en la mesa" << endl;
                num_sentados++;
                break;
            case TAG_LEVANTARSE:
                cout << "--[L]-- Filósofo " << id_filosofo << " se levanta de la mesa" << endl;
                num_sentados--;
                break;
        }

        cout << "Tenemos sentados " << num_sentados << " filósofos" << endl;
    }
}

int main(int argc, char** argv) {
    int id_propio, num_procesos_actual;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &id_propio);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procesos_actual);

    if (num_procesos == num_procesos_actual) {
        // ejecutar la función correspondiente a 'id_propio'
        if (id_propio == id_camarero)
            funcion_camarero();
        else
            if (id_propio % 2 == 0)            // si es par
                funcion_filosofos(id_propio);  //   es un filósofo
            else                               // si es impar
                funcion_tenedores(id_propio);  //   es un tenedor
    } else {
        if (id_propio == 0)  // solo el primero escribe error, indep. del rol
        {
            cout << "el número de procesos esperados es:    " << num_procesos
                 << endl
                 << "el número de procesos en ejecución es: "
                 << num_procesos_actual << endl
                 << "(programa abortado)" << endl;
        }
    }

    MPI_Finalize();
    return 0;
}

// ---------------------------------------------------------------------
