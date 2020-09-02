// Ejercicio propuesto SCD tema 3
// David Cabezas Berrido

// parámeros: M N

/*
  Programa que calcula para una serie de N enteros, cuales son
  múltiplos de los M primeros números primos. N se lee como parámetro
  y los enteros se generan aleatoriamente.
 */

#include <iostream>
#include <thread> // this_thread::sleep_for
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include <mpi.h>
#include <cmath>

using namespace std;
using namespace std::this_thread;
using namespace std::chrono;

int M, N;
const unsigned long long int tope = 10e+16;

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

bool primo(int p){

  double fin = sqrt(p);
  
  for(int i = 2; i <= fin; i++)
    if(p%i==0)
      return false;

  return true;
}

void funcion_etapa(int id)
{
  int p = 2;
  int j = id;
  
  // Calcula el primo correspondiente según id
  while(j > 0){
    p++;
    if(primo(p))
      j--;
  }

  MPI_Status status;
  int valor = 0;

  if(id == 0){                   // Proceso 0 genera los valores

    for(int i = 0; i < N; i++){
      valor = aleatorio<0, tope>();

      if(valor%p == 0)
	MPI_Ssend(&valor, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
    }

    valor = -1;
    MPI_Ssend(&valor, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
  }
  else if(id < M-1){
    MPI_Recv(&valor, 1, MPI_INT, id-1, 0, MPI_COMM_WORLD, &status);
   while(valor >= 0){

     if(valor%p == 0)
       MPI_Ssend(&valor, 1, MPI_INT, id+1, 0, MPI_COMM_WORLD);
     
     MPI_Recv(&valor, 1, MPI_INT, id-1, 0, MPI_COMM_WORLD, &status);
   }
   MPI_Ssend(&valor, 1, MPI_INT, id+1, 0, MPI_COMM_WORLD);
  }

  else{ // Proceso M-1 imprime por pantalla
     MPI_Recv(&valor, 1, MPI_INT, id-1, 0, MPI_COMM_WORLD, &status);
   while(valor >= 0){
     
     if(valor%p == 0)
       cout << valor << endl;
     
     MPI_Recv(&valor, 1, MPI_INT, id-1, 0, MPI_COMM_WORLD, &status);
   }
  }
}

// ---------------------------------------------------------------------

int main( int argc, char *argv[] )
{
  M = atoi(argv[1]);
  N = atoi(argv[2]);
  int id_propio, num_procesos_actual;

   // inicializar MPI, leer identif. de proceso y número de procesos
  MPI_Init( &argc, &argv );
  MPI_Comm_rank( MPI_COMM_WORLD, &id_propio );
  MPI_Comm_size( MPI_COMM_WORLD, &num_procesos_actual );

   if ( M == num_procesos_actual )
   {
     funcion_etapa(id_propio);
   }
   else
   {
      if ( id_propio == 0 ) // solo el primero escribe error, indep. del rol
      { cout << "el número de procesos esperado es:    " << M << endl
             << "el número de procesos en ejecución es: " << num_procesos_actual << endl
             << "(programa abortado)" << endl ;
      }
   }

   // al terminar el proceso, finalizar MPI
   MPI_Finalize( );
   return 0;
}
