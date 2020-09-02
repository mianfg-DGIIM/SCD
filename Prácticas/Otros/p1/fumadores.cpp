#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "Semaphore.h"

using namespace std ;
using namespace SEM ;

static const int num_fumadores = 3;
Semaphore puede_consumir[num_fumadores] = {0,0,0} , puede_colocar = 1;
int num_items = 5;
int mostrador = -1;
bool fin = false;

mutex mtx;
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
  while(num_items>0){

    item = aleatorio<0,num_fumadores-1>();
    
    sem_wait(puede_colocar);
    mtx.lock();
    cout << "Voy a colocar ingrediente " << item << endl;
    mtx.unlock();
    mostrador = item;
    sem_signal(puede_consumir[item]);

    num_items--;
  }

  sem_wait(puede_colocar);
  fin = true;
  for(int j = 0; j < num_fumadores; j++)
    sem_signal(puede_consumir[j]);
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
  int item;
  
   while(true)
   {
     sem_wait(puede_consumir[num_fumador]);
     if(fin) break;
     
     item = mostrador;
     sem_signal(puede_colocar);
     fumar(item);
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
