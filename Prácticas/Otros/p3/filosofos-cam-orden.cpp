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
#include <thread> // this_thread::sleep_for
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include <iostream>

using namespace std;
using namespace std::this_thread ;
using namespace std::chrono ;

const int
num_filosofos = 5,
  num_procesos  = 2*num_filosofos+1,
  sentarse = 1,
  levantarse = 0;


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

void funcion_camarero()
{
  int sentados = 0, tope = num_filosofos-1;
  int etiq_valida, valor;
  int emisor_valido;
  MPI_Status estado;

  int turno = 0;

  int flag;
  
  while(true){
    if(sentados == tope){
  
      etiq_valida = levantarse;
      emisor_valido = MPI_ANY_SOURCE;

      sentados--;
    }
      else{

	while(true){

	  MPI_Iprobe(turno, sentarse, MPI_COMM_WORLD, &flag, &estado); // Compruebo si el que tiene el turno quiere sentarse

	  if(flag > 0){
	    emisor_valido = turno;
	    etiq_valida = sentarse;
	    sentados++;

	    turno=(turno+2)%(num_filosofos*2);

	    break;
	  }

	  MPI_Iprobe(MPI_ANY_SOURCE, levantarse, MPI_COMM_WORLD, &flag, &estado); // Compruebo si alguien quiere levantarse

	  if(flag > 0){
	    emisor_valido = MPI_ANY_SOURCE;
	    etiq_valida = levantarse;
	    sentados--;

	    break;
	  }
	}
      }
    MPI_Recv ( &valor, 1, MPI_INT, emisor_valido, etiq_valida, MPI_COMM_WORLD, &estado );
  }
}

void funcion_filosofos( int id )
{
  int id_ten_izq = (id+1)              % (num_procesos-1), //id. tenedor izq.
    id_ten_der = (id+num_procesos-2) % (num_procesos-1); //id. tenedor der.
  int id_cam = num_procesos-1;
  int peticion;

    while ( true )
      {
	cout <<"Filósofo " <<id << " quiere sentarse a la mesa " <<endl;
	MPI_Ssend(&peticion, 1, MPI_INT, id_cam, sentarse, MPI_COMM_WORLD);
	cout <<"Filósofo " <<id << " solicita ten. izq. " <<id_ten_izq <<endl;
	MPI_Ssend(&peticion, 1, MPI_INT, id_ten_izq, 0, MPI_COMM_WORLD);

	cout <<"Filósofo " <<id <<" solicita ten. der. " <<id_ten_der <<endl;
	MPI_Ssend(&peticion, 1, MPI_INT, id_ten_der, 0, MPI_COMM_WORLD);

	cout <<"Filósofo " <<id <<" comienza a comer" <<endl ;
	sleep_for( milliseconds( aleatorio<150,1500>() ) );

	cout <<"Filósofo " <<id <<" suelta ten. izq. " <<id_ten_izq <<endl;
	MPI_Ssend(&peticion, 1, MPI_INT, id_ten_izq, 0, MPI_COMM_WORLD);

	cout<< "Filósofo " <<id <<" suelta ten. der. " <<id_ten_der <<endl;
	MPI_Ssend(&peticion, 1, MPI_INT, id_ten_der, 0, MPI_COMM_WORLD);

	cout <<"Filósofo " <<id << " quiere levantarse" <<endl;
	MPI_Ssend(&peticion, 1, MPI_INT, id_cam, levantarse, MPI_COMM_WORLD);
	
	cout << "Filosofo " << id << " comienza a pensar" << endl;
	sleep_for( milliseconds( aleatorio<50,500>() ) );
      }  
}
// ---------------------------------------------------------------------

void funcion_tenedores( int id )
{
  int valor, id_filosofo ;  // valor recibido, identificador del filósofo
  MPI_Status estado ;       // metadatos de las dos recepciones

  while ( true )
  {
    MPI_Recv ( &valor, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD,&estado );
    id_filosofo = estado.MPI_SOURCE;
    
    cout <<"\t\t\t\t\tTen. " <<id <<" ha sido cogido por filo. " <<id_filosofo <<endl;

     MPI_Recv ( &valor, 1, MPI_INT, id_filosofo, 0, MPI_COMM_WORLD,&estado );
     cout <<"\t\t\t\t\tTen. "<< id<< " ha sido liberado por filo. " <<id_filosofo <<endl ;
  }
}
// ---------------------------------------------------------------------

int main( int argc, char** argv )
{
   int id_propio, num_procesos_actual ;

   MPI_Init( &argc, &argv );
   MPI_Comm_rank( MPI_COMM_WORLD, &id_propio );
   MPI_Comm_size( MPI_COMM_WORLD, &num_procesos_actual );

   if ( num_procesos == num_procesos_actual )
     {
       // ejecutar la función correspondiente a 'id_propio'
       if(id_propio == num_procesos-1)
	 funcion_camarero(); 
       else if( id_propio % 2 == 0 )          // si es par
         funcion_filosofos( id_propio ); //   es un filósofo
       else                               // si es impar
         funcion_tenedores( id_propio ); //   es un tenedor
     }
   else
     {
       if ( id_propio == 0 ) // solo el primero escribe error, indep. del rol
      { cout << "el número de procesos esperados es:    " << num_procesos << endl
             << "el número de procesos en ejecución es: " << num_procesos_actual << endl
             << "(programa abortado)" << endl ;
      }
   }

   MPI_Finalize( );
   return 0;
}

// ---------------------------------------------------------------------
