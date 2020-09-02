// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Seminario 2. Introducción a los monitores en C++11.
//
// archivo: prodconsN_su_fifo.cpp
// Ejemplo de un monitor en C++11 con semántica SU, para el problema
// del productor/consumidor, con varios y un varios consumidores.
// Opcion FIFO (queue)
//
// -----------------------------------------------------------------------------

#include <iostream>
#include <iomanip>
#include <cassert>
#include <thread>
#include <random>
#include <mutex>
#include "Semaphore.h"
#include "HoareMonitor.h"

using namespace HM ;
using namespace std ;

const int numprod = 2, numcons = 4; // Divisores de num_items

constexpr int
   num_items  = 40 ;     // número de items a producir/consumir

int bloquecons = num_items/numcons, bloqueprod = num_items/numprod;

int contador[numprod]={0};

mutex mtx ;                 // mutex de escritura en pantalla
unsigned
cont_prod[num_items] = {0}, // contadores de verificación: producidos
  cont_cons[num_items] = {0}; // contadores de verificación: consumidos

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

//**********************************************************************
// funciones comunes a las dos soluciones (fifo y lifo)
//----------------------------------------------------------------------

int producir_dato(int k)
{
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   mtx.lock();
   cout << "producido: " << k*bloqueprod+contador[k] << endl << flush ;
   mtx.unlock();
   cont_prod[k*bloqueprod+contador[k]] ++ ;
   return k*bloqueprod+contador[k]++ ;
}
//----------------------------------------------------------------------

void consumir_dato( unsigned dato )
{
   if ( num_items <= dato )
   {
      cout << " dato === " << dato << ", num_items == " << num_items << endl ;
      assert( dato < num_items );
   }
   cont_cons[dato] ++ ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   mtx.lock();
   cout << "                  consumido: " << dato << endl ;
   mtx.unlock();
}
//----------------------------------------------------------------------

void ini_contadores()
{
   for( unsigned i = 0 ; i < num_items ; i++ )
   {  cont_prod[i] = 0 ;
      cont_cons[i] = 0 ;
   }
}

//----------------------------------------------------------------------

void test_contadores()
{
   bool ok = true ;
   cout << "comprobando contadores ...." << flush ;

   for( unsigned i = 0 ; i < num_items ; i++ )
   {
      if ( cont_prod[i] != 1 )
      {
         cout << "error: valor " << i << " producido " << cont_prod[i] << " veces." << endl ;
         ok = false ;
      }
      if ( cont_cons[i] != 1 )
      {
         cout << "error: valor " << i << " consumido " << cont_cons[i] << " veces" << endl ;
         ok = false ;
      }
   }
   if (ok)
      cout << endl << flush << "solución (aparentemente) correcta." << endl << flush ;
}

// *****************************************************************************
// clase para monitor buffer, version FIFO, semántica SU, un prod. y un cons.

class ProdConsNSU : public HoareMonitor
{
private:
  static const int           // constantes:
  num_celdas_total = 10;   //  núm. de entradas del buffer
  int buffer[num_celdas_total],//  buffer de tamaño fijo, con los datos
    primera_libre,          //  indice de celda de la próxima inserción
    primera_ocupada, n;
  CondVar        // colas condicion:
    ocupadas,                //  cola donde esperan los consumidores (n>0)
    libres;                 //  cola donde esperan los productores (n<num_celdas_total)

 public:                    // constructor y métodos públicos
   ProdConsNSU(  ) ;           // constructor
   int  leer();                // extraer un valor (sentencia L) (consumidor)
   void escribir( int valor ); // insertar un valor (sentencia E) (productor)
} ;
// -----------------------------------------------------------------------------

ProdConsNSU::ProdConsNSU(  )
{
  primera_libre = 0;
  primera_ocupada = 0;
  n = 0;
  ocupadas = newCondVar();
  libres = newCondVar();
}
// -----------------------------------------------------------------------------
// función llamada por el consumidor para extraer un dato

int ProdConsNSU::leer(  )
{
   // esperar bloqueado hasta que 0 < num_celdas_ocupadas
   if (n <= 0)
     ocupadas.wait();

   // hacer la operación de lectura, actualizando estado del monitor
   assert( n > 0 );
   const int valor = buffer[primera_ocupada];
   primera_ocupada = (primera_ocupada+1)%num_celdas_total;
   n--;
   
   // señalar al productor que hay un hueco libre, por si está esperando
   libres.signal();
        
   // devolver valor
   return valor ;
}
// -----------------------------------------------------------------------------

void ProdConsNSU::escribir( int valor )
{
  // esperar bloqueado hasta que num_celdas_ocupadas < num_celdas_total
  if (n >= num_celdas_total)
    libres.wait();

  //cout << "escribir: ocup == " << num_celdas_ocupadas << ", total == " << num_celdas_total << endl ;
  assert( n < num_celdas_total );

  // hacer la operación de inserción, actualizando estado del monitor
  buffer[primera_libre] = valor ;
  primera_libre = (primera_libre+1)%num_celdas_total;
  n++;
   
   // señalar al consumidor que ya hay una celda ocupada (por si esta esperando)
   ocupadas.signal();
}
// *****************************************************************************
// funciones de hebras

void funcion_hebra_productora(MRef<ProdConsNSU> monitor, int k)
{
   for( unsigned i = 0 ; i < bloqueprod ; i++ )
   {
      int valor = producir_dato(k) ;
      monitor->escribir( valor );
   }
}
// -----------------------------------------------------------------------------

void funcion_hebra_consumidora(MRef<ProdConsNSU> monitor, int k)
{
   for( unsigned i = 0 ; i < bloquecons ; i++ )
   {
      int valor = monitor->leer();
      consumir_dato( valor ) ;
   }
}
// -----------------------------------------------------------------------------

int main()
{
   cout << "-------------------------------------------------------------------------------" << endl
        << "Problema de los productores-consumidores (N prod/cons, Monitor SU, buffer FIFO). " << endl
        << "-------------------------------------------------------------------------------" << endl << flush;
     
   thread hebra_productora[numprod], hebra_consumidora[numcons];
   MRef<ProdConsNSU> monitor = Create<ProdConsNSU>();

   int i, j;
   
   for(i = 0; i < numprod; i++)
     hebra_productora[i] = thread(funcion_hebra_productora, monitor, i);

   for(j = 0; j < numcons; j++)
     hebra_consumidora[j] = thread(funcion_hebra_consumidora, monitor, j);

   for(j = 0; j < numcons; j++)
     hebra_consumidora[j].join();
   
   for(i = 0; i < numprod; i++)
     hebra_productora[i].join();

   // comprobar que cada item se ha producido y consumido exactamente una vez
   test_contadores() ;
}
