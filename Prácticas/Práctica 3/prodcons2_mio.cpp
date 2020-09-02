// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Práctica 3. Implementación de algoritmos distribuidos con MPI
//
// Archivo: prodcons2.cpp
// Implementación del problema del productor-consumidor con
// un proceso intermedio que gestiona un buffer finito y recibe peticiones
// en orden arbitrario
// (versión con un único productor y un único consumidor)
//
// Historial:
// Actualizado a C++11 en Septiembre de 2017
// -----------------------------------------------------------------------------

#include <iostream>
#include <thread> // this_thread::sleep_for
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include <mpi.h>

using namespace std;
using namespace std::this_thread ;
using namespace std::chrono ;

const int
   id_productor          = 0 ,
   id_buffer             = 1 ,
   id_consumidor         = 2 ,
   num_procesos_esperado = 3 ,
   num_items             = 20,
   tam_vector            = 10;

//**********************************************************************
// plantilla de función para generar un entero aleatorio uniformemente
// distribuido entre dos valores enteros, ambos incluidos
// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
//----------------------------------------------------------------------

template< int min, int max > int aleatorio()
{
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
  return distribucion_uniforme( generador );
}
// ---------------------------------------------------------------------
// ptoducir produce los numeros en secuencia (1,2,3,....)
// y lleva espera aleatorio
int producir()
{
   static int contador = 0 ;
   sleep_for( milliseconds( aleatorio<10,100>()) );
   contador++ ;
   cout << "Productor ha producido valor " << contador << endl << flush;
   return contador ;
}
// ---------------------------------------------------------------------

void funcion_productor()
{
   for ( int i = 0; i < num_items; i++ ) {
        int dato = producir();
        cout << "Productor enviará valor " << dato << flush << endl;
        MPI_Ssend(&dato, 1, MPI_INT, id_buffer, 0, MPI_COMM_WORLD);
    }
}
// ---------------------------------------------------------------------

void consumir( int valor_cons )
{
   // espera bloqueada
   sleep_for( milliseconds( aleatorio<110,200>()) );
   cout << "Consumidor ha consumido valor " << valor_cons << endl << flush ;
}
// ---------------------------------------------------------------------

void funcion_consumidor()
{
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

void funcion_buffer()
{
    int buffer[tam_vector],
        valor,
        primera_libre = 0,
        primera_ocupada = 0,
        num_celdas_ocupadas = 0,
        id_emisor_aceptable;
    MPI_Status estado;

    for ( int i = 0; i < num_items*2; i++ ) {
        // 1. determinar si se puede enviar solo prod, solo cons o todos
        if ( num_celdas_ocupadas == 0 )
            id_emisor_aceptable = id_productor;
        else if ( num_celdas_ocupadas == tam_vector )
            id_emisor_aceptable = id_consumidor;
        else
            id_emisor_aceptable = MPI_ANY_SOURCE;

        // 2. recibir mensaje de un emisor o emisores aceptables
        MPI_Recv(&valor, 1, MPI_INT, id_emisor_aceptable, 0, MPI_COMM_WORLD, &estado);
        
        // 3. procesar mensaje recibido
        switch ( estado.MPI_SOURCE ) {
            case id_productor:
                buffer[primera_libre] = valor;
                primera_libre = (primera_libre+1)%tam_vector;
                num_celdas_ocupadas++;
                cout << "Buffer ha recibido el valor: " << valor << endl << flush;
                break;
            case id_consumidor:
                valor = buffer[primera_ocupada];
                primera_ocupada = (primera_ocupada+1)%tam_vector;
                num_celdas_ocupadas--;
                cout << "Buffer va a enviar el valor: " << valor << endl << flush;
                MPI_Ssend(&valor, 1, MPI_INT, id_consumidor, 0, MPI_COMM_WORLD);
                break;
        }
    }
}

// ---------------------------------------------------------------------

int main( int argc, char *argv[] )
{
   int id_propio, num_procesos_actual;

   // inicializar MPI, leer identif. de proceso y número de procesos
   MPI_Init( &argc, &argv );
   MPI_Comm_rank( MPI_COMM_WORLD, &id_propio );
   MPI_Comm_size( MPI_COMM_WORLD, &num_procesos_actual );

   if ( num_procesos_esperado == num_procesos_actual )
   {
      // ejecutar la operación apropiada a 'id_propio'
      if ( id_propio == id_productor )
         funcion_productor();
      else if ( id_propio == id_buffer )
         funcion_buffer();
      else
         funcion_consumidor();
   }
   else
   {
      if ( id_propio == 0 ) // solo el primero escribe error, indep. del rol
      { cout << "el número de procesos esperados es:    " << num_procesos_esperado << endl
             << "el número de procesos en ejecución es: " << num_procesos_actual << endl
             << "(programa abortado)" << endl ;
      }
   }

   // al terminar el proceso, finalizar MPI
   MPI_Finalize( );
   return 0;
}
