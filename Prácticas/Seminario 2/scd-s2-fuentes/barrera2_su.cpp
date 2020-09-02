// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Seminario 2. Introducción a los monitores en C++11.
//
// archivo: barrera2_su.cpp
// Ejemplo de un monitor 'Barrera' parcial, con semántica SU
//
// Historial:
// Creado en Julio de 2017
// -----------------------------------------------------------------------------


#include <iostream>
#include <iomanip>
#include <random>
#include "HoareMonitor.h"

using namespace std ;
using namespace HM ;

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

// *****************************************************************************
// clase para monitor Barrera, version 2,  semántica SU

class MBarreraParSU : public HoareMonitor
{
   private:
   int      cont,             // contador de hebras en cita
            num_hebras_cita ; // número total de hebras en cita
   CondVar  cola ;            // cola de hebras esperando en cita

   public:
   MBarreraParSU( int p_num_hebras_cita ) ; // constructor
   void cita( int num_hebra );               // método de cita
} ;
// -----------------------------------------------------------------------------

MBarreraParSU::MBarreraParSU( int p_num_hebras_cita )
{
   num_hebras_cita = p_num_hebras_cita ;
   cont            = 0 ;
   cola            = newCondVar();
}
// -----------------------------------------------------------------------------

void MBarreraParSU::cita( int num_hebra )
{
   cont ++ ;
   const int orden = cont ;

   cout <<  "Llega hebra " <<setw(2) << num_hebra << " (" <<setw(2) << orden << ")." << endl ;
   if ( cont < num_hebras_cita )
      cola.wait();
   else
   {
      for( int i = 0 ; i < num_hebras_cita-1 ; i++ )
         cola.signal() ;
      cont = 0 ;
   }
   cout << "              Sale hebra " <<setw(2) << num_hebra << " (" <<setw(2) << orden << ")." << endl ;
}
// -----------------------------------------------------------------------------

void funcion_hebra( MRef<MBarreraParSU> monitor, int num_hebra )
{
   while( true )
   {
      const int ms = aleatorio<0,30>();
      this_thread::sleep_for( chrono::milliseconds(ms) );
      monitor->cita( num_hebra );
   }

}
// *****************************************************************************

int main()
{
   cout <<  "Barrera parcial SU: inicio simulación." << endl ;

   // declarar el número total de hebras
   const int num_hebras      = 1000,
             num_hebras_cita = 10 ;

   // crear monitor  ('monitor' es una referencia al mismo, de tipo MRef<...>)
   MRef<MBarreraParSU> monitor = Create<MBarreraParSU>( num_hebras_cita );

   // crear y lanzar hebras
   thread hebra[num_hebras];
   for( unsigned i = 0 ; i < num_hebras ; i++ )
      hebra[i] = thread( funcion_hebra, monitor, i );

   // esperar a que terminen las hebras (no pasa nunca)
   for( unsigned i = 0 ; i < num_hebras ; i++ )
      hebra[i].join();
}
