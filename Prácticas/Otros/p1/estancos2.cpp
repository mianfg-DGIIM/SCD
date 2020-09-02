#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "Semaphore.h"

using namespace std ;
using namespace SEM ;

static const int num_fumadores = 4;
Semaphore puede_consumir[num_fumadores] = {0,0,0,0} , puede_colocar = 1;

mutex mtx; // Para que la salida por pantalla sea clara

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

//----------------------------------------------------------------------
// función que ejecuta la hebra del estanquero

void funcion_hebra_estanquero(  )
{
  int item;
  while(true){

    item = aleatorio<0,num_fumadores-1>();
    
    sem_wait(puede_colocar);
    mtx.lock();
    cout << "Estanquero: Coloco ingrediente " << item << endl;
    mtx.unlock();
    sem_signal(puede_consumir[item]);
  }
}

//-------------------------------------------------------------------------
// Función que simula la acción de fumar, como un retardo aleatoria de la hebra

void fumar( int num_fumador )
{

   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_fumar( aleatorio<20,200>() );

   // informa de que comienza a fumar

   mtx.lock();
    cout << "Fumador " << num_fumador << "  :"
          << " empieza a fumar (" << duracion_fumar.count() << " milisegundos)" << endl;
    mtx.unlock();
   // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
   this_thread::sleep_for( duracion_fumar );

   // informa de que ha terminado de fumar

   mtx.lock();
    cout << "Fumador " << num_fumador << "  : termina de fumar, comienza espera de ingrediente." << endl;
    mtx.unlock();

}

//----------------------------------------------------------------------
// función que ejecuta la hebra del fumador
void  funcion_hebra_fumador( int num_fumador )
{
  int fumados = 0;
  
   while(true)
   {
     sem_wait(puede_consumir[num_fumador]);

     if(fumados%2){ // Lleva un número impar de cigarros fumados
       sem_signal(puede_colocar);
       fumar(num_fumador);
       fumados++;
     }
     else{ // Lleva un número par de cigarros fumados 
       fumar(num_fumador);
       fumados++;
       sem_signal(puede_colocar);
     }
   }
}

//----------------------------------------------------------------------

int main()
{
  thread hebra_fumador[num_fumadores], hebra_estanquero(funcion_hebra_estanquero);

  
  for(int i = 0; i < num_fumadores; i++)
    hebra_fumador[i] = thread(funcion_hebra_fumador,i);

  for(int i = 0; i < num_fumadores; i++)
    hebra_fumador[i].join();
  
  hebra_estanquero.join();
}
