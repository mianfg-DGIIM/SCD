/**
 * 
 */

#include <iostream>
#include <thread>
#include <random>
#include <chrono>
#include <mpi.h>

using namespace std;
using namespace std::this_thread;
using namespace std::chrono;

#define TAG_PRODUCTOR  1
#define TAG_CONSUMIDOR 2

const int
    num_productores     = 4,
    num_consumidores    = 5,
    id_buffer           = num_productores,
    num_procs_esperado  = num_productores + num_consumidores + 1,
    num_items           = num_productores*num_consumidores*2,
        // debe ser múltiplo de num_productores, num_consumidores
    tam_vector          = 10;

template<int min, int max> int aleatorio() {
    static default_random_engine generador( (random_device())() );
    static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
    return distribucion_uniforme( generador );
}

int producir(int id_productor) {
    static int contador = id_productor*100; // num_productor*num_items/num_productores  NOTA: num_items/num_productores = nº items por productor
    sleep_for( milliseconds(aleatorio<10,100>()) );
    contador++;
    cout << "Productor " << id_productor << " ha producido valor: " << contador << endl << flush;
    return contador;
}

int consumir(int valor_cons, int id_consumidor) {
    sleep_for( milliseconds(aleatorio<110,200>()) );
    cout << "Consumidor " << id_consumidor << " ha consumido valor " << valor_cons << endl << flush;
}

void funcion_productor(int id_productor) {
    for ( unsigned int i = 0; i < num_items/num_productores; i++ ) {
        int valor_prod = producir(id_productor);
        cout << "Productor " << id_productor << " va a enviar valor " << valor_prod << endl << flush;
        MPI_Ssend(&valor_prod, 1, MPI_INT, id_buffer, TAG_PRODUCTOR, MPI_COMM_WORLD);
    }
}

void funcion_buffer() {
    int
        buffer[tam_vector],
        valor,
        primera_libre       = 0,
        primera_ocupada     = 0,
        num_celdas_ocupadas = 0,
        tag_aceptable;
    MPI_Status estado;

    for ( unsigned int i = 0; i < num_items*2; i++ ) {
        // 1. determinar quién puede enviar
        if ( num_celdas_ocupadas == 0 )
            tag_aceptable = TAG_PRODUCTOR;
        else if ( num_celdas_ocupadas == tam_vector )
            tag_aceptable = TAG_CONSUMIDOR;
        else
            tag_aceptable = MPI_ANY_TAG;

        // 2. recibir un mensaje del emisor o emisores aceptables
        MPI_Recv(&valor, 1, MPI_INT, MPI_ANY_SOURCE, tag_aceptable, MPI_COMM_WORLD, &estado);

        // 3. procesar el mensaje recibido
        switch(estado.MPI_TAG) {    // leer emisor del mensaje en metadatos
            case TAG_PRODUCTOR:
                buffer[primera_libre] = valor;
                primera_libre = (primera_libre+1)%tam_vector;
                num_celdas_ocupadas++;
                cout << "Buffer ha recibido valor " << valor << endl << flush;
                break;
            case TAG_CONSUMIDOR:
                valor = buffer[primera_ocupada];
                primera_ocupada = (primera_ocupada+1)%tam_vector;
                num_celdas_ocupadas--;
                cout << "Buffer va a enviar valor " << valor << endl << flush;
                MPI_Ssend(&valor, 1, MPI_INT, estado.MPI_SOURCE, TAG_CONSUMIDOR, MPI_COMM_WORLD);
                break;
        }
    }
}

void funcion_consumidor(int id_consumidor) {
    int peticion, valor_rec;
    MPI_Status estado;

    for ( unsigned int i = 0; i < num_items/num_consumidores; i++ ) {
        MPI_Ssend(&peticion, 1, MPI_INT, id_buffer, TAG_CONSUMIDOR, MPI_COMM_WORLD);
        MPI_Recv(&valor_rec, 1, MPI_INT, id_buffer, TAG_CONSUMIDOR, MPI_COMM_WORLD, &estado);
        cout << "Consumidor " << id_consumidor << " ha recibido valor " << valor_rec << endl << flush;
        consumir(valor_rec, id_consumidor);
    }
}


int main( int argc, char** argv ) {
    int id_propio, num_procs_actual;

    // inicializar MPI, leer identif. de proceso y núm. de procesos
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &id_propio);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs_actual);

    if ( num_items % num_productores != 0 || num_items % num_consumidores != 0 ) {
        cout << "El número de ítems debe ser múltiplo del número de productores "
             << "y del número de consumidores" << endl;
        return 1;
    }

    if ( num_procs_actual == num_procs_esperado ) {
        if ( id_propio >= 0 && id_propio < id_buffer )
            funcion_productor(id_propio   + 1);
        else if ( id_propio == id_buffer )
            funcion_buffer();
        else
            funcion_consumidor(id_propio - id_buffer - 1   + 1);
    } else {
        cout << "Se espera un número de procesos que sea suma del número "
             << "de productores (" << num_productores << ") y del número de "
             << "consumidores (" << num_consumidores << "), más uno" << endl;
    }

    MPI_Finalize();
    return 0;
}