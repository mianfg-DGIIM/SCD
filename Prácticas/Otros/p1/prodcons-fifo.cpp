#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random>
#include "Semaphore.h"

using namespace std ;
using namespace SEM ;

//**********************************************************************
// variables compartidas

const int num_items = 40 ,   // número de items
  tam_vec   = 10;   // tamaño del buffer

int in = 0, out = 0;

unsigned  cont_prod[num_items] = {0}, // contadores de verificación: producidos
  cont_cons[num_items] = {0}, // contadores de verificación: consumidos
  buf[tam_vec] = {0};

Semaphore puede_prod(tam_vec), puede_cons(0); // para controlar que no se lea con el buffer vacío ni se escriba con el buffer lleno

mutex mtx_output;
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

int producir_dato()
{
   static int contador = 0 ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));

   mtx_output.lock();
   cout << "producido: " << contador << endl << flush ;
   mtx_output.unlock();

   cont_prod[contador] ++ ;
   return contador++ ;
}
//----------------------------------------------------------------------

void consumir_dato( unsigned dato )
{
   assert( dato < num_items );
   cont_cons[dato] ++ ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));

   mtx_output.lock();
   cout << "                  consumido: " << dato << endl ;
   mtx_output.unlock();   
}

//----------------------------------------------------------------------

void test_contadores()
{
   bool ok = true ;
   cout << "comprobando contadores ...." ;
   for( unsigned i = 0 ; i < num_items ; i++ )
   {  if ( cont_prod[i] != 1 )
      {  cout << "error: valor " << i << " producido " << cont_prod[i] << " veces." << endl ;
         ok = false ;
      }
      if ( cont_cons[i] != 1 )
      {  cout << "error: valor " << i << " consumido " << cont_cons[i] << " veces" << endl ;
         ok = false ;
      }
   }
   if (ok)
      cout << endl << flush << "solución (aparentemente) correcta." << endl << flush ;
}

//----------------------------------------------------------------------

void  funcion_hebra_productora(  )
{

  int dato;
  
   for( unsigned i = 0 ; i < num_items ; i++ )
   {
      dato = producir_dato() ;
      
      sem_wait(puede_prod);
      
      buf[in] = dato;
      in = (in+1)%tam_vec;

      sem_signal(puede_cons);
   }
}

//----------------------------------------------------------------------

void funcion_hebra_consumidora(  )
{

  int dato;
  
   for( unsigned i = 0 ; i < num_items ; i++ )
   {
     sem_wait(puede_cons);

     dato = buf[out];
     out = (out+1)%tam_vec;
       
     sem_signal(puede_prod);
     
     consumir_dato( dato ) ;
    }
}
//----------------------------------------------------------------------

int main()
{
   cout << "--------------------------------------------------------" << endl
        << "Problema de los productores-consumidores (solución FIFO)." << endl
        << "--------------------------------------------------------" << endl
        << flush ;

   thread hebra_productora ( funcion_hebra_productora ),
          hebra_consumidora( funcion_hebra_consumidora );

   hebra_productora.join() ;
   hebra_consumidora.join() ;

   test_contadores();
}
