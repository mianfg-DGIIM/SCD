// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Práctica 3. Implementación de algoritmos distribuidos con MPI
//
// Archivo: filosofos-cam.cpp
// Implementación del problema de los filósofos.
//
// Se evita el interbloqueo mediante un proceso 'camarero' que da permiso
// a los filósofos para sentarse y levantarse de la mesa mediante
// espera selectiva.
//
// Antonio Coín Castro
//
// -----------------------------------------------------------------------------

#include <mpi.h>
#include <thread>
#include <random>
#include <chrono>
#include <iostream>

using namespace std;
using namespace std::this_thread;
using namespace std::chrono;

//**********************************************************************
// Variables globales
//----------------------------------------------------------------------

// Parámetros del programa

const int
   num_filosofos = 5,
   num_procesos_efectivos = 2 * num_filosofos, // un tenedor por cada filósofo
   num_procesos_esperados  = num_procesos_efectivos + 1,  // Hay un camarero
   id_camarero = num_procesos_efectivos;

// Etiquetas MPI

const int
   etiq_sentarse = 0,
   etiq_levantarse = 1;

//**********************************************************************
// Plantilla de función para generar un entero aleatorio uniformemente
// distribuido en el rango [min, max]
//----------------------------------------------------------------------

template< int min, int max > int aleatorio()
{
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max );
  return distribucion_uniforme( generador );
}

//**********************************************************************
// Función que simula la acción de comer o pensar
//----------------------------------------------------------------------

void retraso_aleatorio( string mensaje, int id )
{
  cout << "--- Filósofo " << id << " " << mensaje << " ---" << endl;
  sleep_for( milliseconds( aleatorio<10,100>() ) );
}

//**********************************************************************
// Funciones que implementan el paso de mensajes
//----------------------------------------------------------------------

void funcion_filosofos( int id )
{
  int id_ten_izq = (id + 1) % num_procesos_efectivos,
      id_ten_der = (id + num_procesos_efectivos - 1) % num_procesos_efectivos,
      peticion;

  while ( true )
  {
    // 1. Sentarse

    cout << "Filósofo " << id << " solicita permiso para sentarse a la mesa" << endl;
    MPI_Ssend( &peticion, 1, MPI_INT, id_camarero, etiq_sentarse, MPI_COMM_WORLD );

    // 2. Pedir tenedores

    cout << "Filósofo " << id << " solicita tenedor izquierdo (" << id_ten_izq << ")" << endl;
    MPI_Ssend( &peticion, 1, MPI_INT, id_ten_izq, 0, MPI_COMM_WORLD );

    cout << "Filósofo " << id << " solicita tenedor derecho (" << id_ten_der << ")" << endl;
    MPI_Ssend( &peticion, 1, MPI_INT, id_ten_der, 0, MPI_COMM_WORLD );

    // 3. Comer

    retraso_aleatorio("comienza a comer", id);

    // 4. Soltar tenedores

    cout << "Filósofo " << id << " suelta tenedor izquierdo (" << id_ten_izq << ")" << endl;
    MPI_Ssend( &peticion, 1, MPI_INT, id_ten_izq, 0, MPI_COMM_WORLD );

    cout << "Filósofo " << id <<" suelta tenedor derecho (" << id_ten_der << ")" << endl;
    MPI_Ssend( &peticion, 1, MPI_INT, id_ten_der, 0, MPI_COMM_WORLD );

    // 5. Levantarse

    cout << "Filósofo " << id << " solicita permiso para levantarse" << endl;
    MPI_Ssend( &peticion, 1, MPI_INT, id_camarero, etiq_levantarse, MPI_COMM_WORLD );

    // 6. Pensar

    retraso_aleatorio("comienza a pensar", id);
  }
}

// ---------------------------------------------------------------------

void funcion_camarero()
{
  int        filosofos_sentados = 0,  // número de filósofos sentados en la mesa
             peticion,                // petición recibida
             id_filosofo,             // filósofo que realiza la petición
             etiqueta_aceptable;      // identificador de etiqueta aceptable
  MPI_Status estado;                  // metadatos del mensaje recibido

  while( true )
  {
     // 1. Determinar si atiende peticiones de levantarse o de sentarse y levantarse

     if ( filosofos_sentados < num_filosofos - 1 )  // si hay $num_filosofos-2$ o menos
        etiqueta_aceptable = MPI_ANY_TAG;       // $~~~$ cualquiera
     else                                       // si mesa llena
        etiqueta_aceptable = etiq_levantarse;   // $~~~$ solo levantarse

     // 2. Recibir petición con etiqueta aceptable

     MPI_Recv( &peticion, 1, MPI_INT, MPI_ANY_SOURCE, etiqueta_aceptable, MPI_COMM_WORLD, &estado );
     id_filosofo = estado.MPI_SOURCE;

     // 3. Procesar el mensaje recibido

     switch( estado.MPI_TAG ) // leer etiqueta del mensaje en metadatos
     {
        case etiq_levantarse:
           cout << "\tFilósofo " << id_filosofo << " se levanta de la mesa" << endl;
           filosofos_sentados--;
           break;

        case etiq_sentarse:
           cout << "\tFilósofo " << id_filosofo << " se sienta a la mesa" << endl;
           filosofos_sentados++;
           break;
     }

     cout << "\t --- Actualmente hay " << filosofos_sentados << " filósofos sentados --- " << endl;
  }
}

// ---------------------------------------------------------------------

void funcion_tenedores( int id )
{
  int        valor,
             id_filosofo ;
  MPI_Status estado ;

  while ( true )
  {
     // Recibir petición de cualquier filósofo
     MPI_Recv( &valor, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &estado );
     id_filosofo = estado.MPI_SOURCE;
     cout << "\t\tTenedor " << id <<" cogido por filósofo " << id_filosofo << endl;

     // Recibir liberación de filósofo 'id_filosofo'
     MPI_Recv( &valor, 1, MPI_INT, id_filosofo, 0, MPI_COMM_WORLD, &estado );
     cout << "\t\tTenedor " << id << " liberado por filósofo " << id_filosofo << endl;
  }
}

//**********************************************************************
// Main
//----------------------------------------------------------------------

int main( int argc, char** argv )
{
   int id_propio, num_procesos_actual;

   MPI_Init( &argc, &argv );
   MPI_Comm_rank( MPI_COMM_WORLD, &id_propio );
   MPI_Comm_size( MPI_COMM_WORLD, &num_procesos_actual );

   if ( num_procesos_esperados == num_procesos_actual )
   {
      if ( id_propio == id_camarero )
         funcion_camarero();
      else if ( id_propio % 2 == 0 )     // si es par
         funcion_filosofos( id_propio ); // es un filósofo
      else                               // si es impar
         funcion_tenedores( id_propio ); // es un tenedor
   }
   else
   {
      if ( id_propio == 0 )
      {
        cout << "error: el número de procesos esperados es " << num_procesos_esperados
             << ", pero el número de procesos en ejecución es: " << num_procesos_actual << endl;
      }
   }

   MPI_Finalize();
   return 0;
}