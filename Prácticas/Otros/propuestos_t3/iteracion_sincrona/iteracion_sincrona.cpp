// Ejercicio propuesto SCD tema 3
// David Cabezas Berrido

/*
  Programa que calcula transformaciones iterativas en un vector de 12
  componentes
 */

// Parámetros:
// p: número de procesos en los que se divide el vector, debe ser divisor de 12
// M: número de iteraciones

#include <iostream>
#include <thread> // this_thread::sleep_for
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include <mpi.h>
#include <cmath>

using namespace std;
using namespace std::this_thread;
using namespace std::chrono;

const int N = 12;
int M, p;
const double valores[] = {18.8, 23.5, 17.9, 21.3, 14.5, -5.1, 0.17, -24.7, 3.0, 41.1, 68.0, -17.3};

const int tag_imprimir = 1; // Para confirmar que impriman en orden 

void tarea(int id)
{
  double* bloque = new double[N/p+2];
  int i, j, k;
  
  for(i = id*N/p, j = 1; j < N/p+1; i++, j++)
    bloque[j] = valores[i];
  
  MPI_Status status;

  double aux, izquierda;

  for(k = 0; k < M; k++){
    // Comunicar valores extremos con vecinos
    MPI_Send(bloque+1, 1, MPI_DOUBLE, (id-1+p)%p, 0, MPI_COMM_WORLD);
    MPI_Send(bloque+N/p, 1, MPI_DOUBLE, (id+1)%p, 0, MPI_COMM_WORLD);
    MPI_Recv(bloque, 1, MPI_DOUBLE, (id-1+p)%p, 0, MPI_COMM_WORLD, &status);
    MPI_Recv(bloque+N/p+1, 1, MPI_DOUBLE, (id+1)%p, 0, MPI_COMM_WORLD, &status);
   
    // Actualizar entradas
    izquierda = bloque[0]; // Valor de la izquierda
    for(j = 1; j < N/p+1; j++){
      aux = bloque[j];
      bloque[j] = (izquierda-bloque[j]+bloque[j+1])/2;
      izquierda = aux;
    }
  }

  // Imprimir por pantalla: cada proceso debe esperar a que el anterior imprima
  if(id == 0){
    for(j = 1; j < N/p+1; j++)
      cout << bloque[j] << ", ";
    cout.flush();
    MPI_Ssend(&aux, 1, MPI_DOUBLE, 1, tag_imprimir, MPI_COMM_WORLD);
  }
  else if(id == p-1){
    MPI_Recv(&aux, 1, MPI_DOUBLE, p-2, tag_imprimir, MPI_COMM_WORLD, &status);
    for(j = 1; j < N/p; j++)
      cout << bloque[j] << ", ";
    cout << bloque[N/p] << endl;
    cout.flush();
  }
  else{
    MPI_Recv(&aux, 1, MPI_DOUBLE, id-1, tag_imprimir, MPI_COMM_WORLD, &status);
    for(j = 1; j < N/p+1; j++)
      cout << bloque[j] << ", ";
    cout.flush();
    MPI_Ssend(&aux, 1, MPI_DOUBLE, id+1, tag_imprimir, MPI_COMM_WORLD);
  }

  delete [] bloque;
}

// ---------------------------------------------------------------------

int main( int argc, char *argv[] )
{
  p = atoi(argv[1]);
  M = atoi(argv[2]);
  
  int id_propio, num_procesos_actual;

   // inicializar MPI, leer identif. de proceso y número de procesos
  MPI_Init( &argc, &argv );
  MPI_Comm_rank( MPI_COMM_WORLD, &id_propio );
  MPI_Comm_size( MPI_COMM_WORLD, &num_procesos_actual );

   if (p > 2 and N%p==0 and num_procesos_actual==p)
   {
     tarea(id_propio);
   }
   else
   {
      if ( id_propio == 0 ) // solo el primero escribe error, indep. del rol
	{ cout << "El número de procesos debe ser mayor que 2, divisor de 12 y debe coincidir con el número de subvectores" << endl
             << "(programa abortado)" << endl ;
      }
   }

   // al terminar el proceso, finalizar MPI
   MPI_Finalize( );
   return 0;
}
