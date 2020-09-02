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

const int np = 1,
  nc = 4,
  num_items             = np*nc*10, // Múltiplo de np y nc
  tam_vector            = 3,
  bloqueprod = num_items/np,
  bloquecons = num_items/nc;

const int etiq_prod = 2, etiq_cons_impar = 1, etiq_cons_par = 0;

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
int producir(int id)
{
   static int contador = id*bloqueprod ;
   sleep_for( milliseconds( aleatorio<10,100>()) );
   contador++ ;
   cout << "Productor " << id << " ha producido valor " << contador << endl;
   cout.flush();
   return contador ;
}
// ---------------------------------------------------------------------

void funcion_productor(int id)
{
   for ( unsigned int i= 0 ; i < bloqueprod ; i++ )
   {
      // producir valor
      int valor_prod = producir(id);
      // enviar valor
      cout << "Productor " << id << " va a enviar valor " << valor_prod << endl;
      cout.flush();
      MPI_Ssend( &valor_prod, 1, MPI_INT, np, etiq_prod, MPI_COMM_WORLD );
   }
}
// ---------------------------------------------------------------------

void consumir( int id, int valor_cons )
{
   // espera bloqueada
   sleep_for( milliseconds( aleatorio<110,200>()) );
   cout << "\t\t\t\t\t\tConsumidor " << id << " ha consumido valor " << valor_cons << endl;
   cout.flush();
}
// ---------------------------------------------------------------------

void funcion_consumidor(int id)
{
   int         peticion,
     valor_rec = 1,
     tag = id%2;
   
   MPI_Status  estado ;

   for( unsigned int i=0 ; i < bloquecons; i++ )
   {
      MPI_Ssend( &peticion,  1, MPI_INT, np, tag, MPI_COMM_WORLD);
      MPI_Recv ( &valor_rec, 1, MPI_INT, np, tag, MPI_COMM_WORLD,&estado);
      cout << "\t\t\t\t\t\tConsumidor " << id << " ha recibido valor " << valor_rec << endl;
      cout.flush();
      consumir( id, valor_rec );
   }
}
// ---------------------------------------------------------------------

void funcion_buffer()
{
  int buffer[tam_vector],    // buffer con celdas ocupadas y vacías
    valor,                   // valor recibido o enviado
    primera_libre       = 0, // índice de primera celda libre
    primera_ocupada     = 0, // índice de primera celda ocupada
    num_celdas_ocupadas = 0, // número de celdas ocupadas
    etiq_emisor_aceptable,   // etiqueta de los emisores aceptables
    flag,
    etiq_cons_valido;
   MPI_Status estado ;                 // metadatos del mensaje recibido

   int id_fuente;

   bool encontrado;

   for( unsigned int i=0 ; i < num_items*2 ; i++ )
   {
     etiq_cons_valido = buffer[primera_ocupada]%2;
     
      // 1. determinar si puede enviar solo prod., solo cons, o todos
      if ( num_celdas_ocupadas == 0 )               // si buffer vacío
         etiq_emisor_aceptable = etiq_prod ;       // $~~~$ solo prod.
      
      else if ( num_celdas_ocupadas == tam_vector ) // si buffer lleno
	etiq_emisor_aceptable = etiq_cons_valido;
      
      else{ // Acepto tanto productor como consumidor (de paridad adecuada)

	encontrado = false;

	while(!encontrado){

	  // Pruebo con productor
	  MPI_Iprobe(MPI_ANY_SOURCE, etiq_prod, MPI_COMM_WORLD, &flag, &estado);
	  if(flag > 0){
	    encontrado = true;
	    etiq_emisor_aceptable = etiq_prod;
	  }
	  // Pruebo con consumidor
	  MPI_Iprobe(MPI_ANY_SOURCE, etiq_cons_valido, MPI_COMM_WORLD, &flag, &estado);

	  if(flag > 0){
	    encontrado = true;
	    etiq_emisor_aceptable = etiq_cons_valido;
	  }
	}
      }

      // 2. recibir un mensaje del emisor o emisores aceptables

      MPI_Recv( &valor, 1, MPI_INT, MPI_ANY_SOURCE, etiq_emisor_aceptable, MPI_COMM_WORLD, &estado );

      // 3. procesar el mensaje recibido

      id_fuente = estado.MPI_SOURCE;

      if(id_fuente < np){ // si ha sido el productor: insertar en buffer
	buffer[primera_libre] = valor ;
	primera_libre = (primera_libre+1) % tam_vector ;
	num_celdas_ocupadas++ ;
	cout << "\t\t\tBuffer ha recibido valor " << valor << endl ;
	cout.flush();
      }

      else { // if(id_fuente > np) {  // si ha sido un consumidor: extraer y enviarle
	valor = buffer[primera_ocupada] ;
	primera_ocupada = (primera_ocupada+1) % tam_vector ;
	num_celdas_ocupadas-- ;
	cout << "\t\t\tBuffer va a enviar valor " << valor << " a " << id_fuente-np-1 << endl ;
	cout.flush();
	MPI_Ssend( &valor, 1, MPI_INT, id_fuente, valor%2, MPI_COMM_WORLD);
      }
   }
}

// ---------------------------------------------------------------------

int main( int argc, char *argv[] )
{

  int num_procesos_esperado = np+nc+1;
   int id_propio, num_procesos_actual;

   // inicializar MPI, leer identif. de proceso y número de procesos
   MPI_Init( &argc, &argv );
   MPI_Comm_rank( MPI_COMM_WORLD, &id_propio );
   MPI_Comm_size( MPI_COMM_WORLD, &num_procesos_actual );

   if ( num_procesos_esperado == num_procesos_actual )
   {
      // ejecutar la operación apropiada a 'id_propio'
      if ( id_propio < np )
         funcion_productor(id_propio);
      else if ( id_propio == np )
         funcion_buffer();
      else
         funcion_consumidor(id_propio-np-1);
   }
   else
   {
      if ( id_propio == 0 ) // solo el primero escribe error, indep. del rol
      { cout << "el número de procesos esperado es:    " << num_procesos_esperado << endl
             << "el número de procesos en ejecución es: " << num_procesos_actual << endl
             << "(programa abortado)" << endl ;
      }
   }

   // al terminar el proceso, finalizar MPI
   MPI_Finalize( );
   return 0;
}
