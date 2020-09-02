/**
 * Examen Práctica 3 - SCD
 * Alumno: Miguel Ángel Fernández Gutiérrez
 * 12 diciembre, 2019
 */
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
        peticion, tamanio;
    
    int vector[5] = {0, 0, 0, 0, 0}; // valores iniciales no importan

    while (true) {
        tamanio = aleatorio<1, 5>();
        cout << "Filósofo " << id << " solicita sentarse en mesa" << endl;
        MPI_Ssend(&vector, tamanio, MPI_INT, id_camarero, TAG_SENTARSE, MPI_COMM_WORLD);

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
    int valor, id_filosofo, num_sentados = 0, etiq_aceptable,
        propina = 0,                // propina acumulada
        no_ciclos_necesarios = 1,   // número de ciclos módulo 10 necesarios para mostrar mensaje
        tamanio_recibido,           // para leer el mensaje previo probe
        propina_recibida;           // propina enviada en cada petición
    MPI_Status estado;

    while (true) {
        // determinar si atiende a peticiones de sentarse y levantarse o sólo de levantarse
        if ( num_sentados < num_filosofos - 1 ) {
            etiq_aceptable = MPI_ANY_TAG;
        } else {
            etiq_aceptable = TAG_LEVANTARSE;
        }
        
        // pedir una petición
        MPI_Probe(MPI_ANY_SOURCE, etiq_aceptable, MPI_COMM_WORLD, &estado);
        id_filosofo = estado.MPI_SOURCE;
        MPI_Get_count(&estado, MPI_INT, &tamanio_recibido);
        propina_recibida = tamanio_recibido;
        MPI_Recv(&valor, tamanio_recibido, MPI_INT, id_filosofo, etiq_aceptable, MPI_COMM_WORLD, &estado);

        // procesar mensaje
        if ( estado.MPI_TAG == TAG_SENTARSE ) {
            propina += propina_recibida;
            num_sentados++;
            
            if ( propina > no_ciclos_necesarios*10 ) {
                no_ciclos_necesarios++;
                cout << "¡¡¡He alcanzado ya en total " << propina << " unidades de propina!!!" << endl;
            }
        } else {    // estado.MPI_TAG == TAG_LEVANTARSE
            num_sentados--;
        }
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
                funcion_filosofos(id_propio/2);  //   es un filósofo
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
