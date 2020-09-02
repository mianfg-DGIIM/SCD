#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "Semaphore.h"

using namespace std ;
using namespace SEM ;

// Hay que inicializar los vectores manualmente si se cambia alguna de las constantes
static const int num_fumadores = 3; // Por estanco
static const int num_estancos = 2;
Semaphore puede_consumir[num_estancos][num_fumadores] = {{0,0,0},{0,0,0}};
Semaphore puede_colocar[num_estancos] = {1,1};
int num_items[num_estancos] = {3,5};
int mostrador[num_estancos] = {-1,-1};
bool fin[num_estancos] = {false,false};

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

//----------------------------------------------------------------------
// función que ejecuta la hebra del estanquero

void funcion_hebra_estanquero(int num_estanco)
{
  int item;
  while(num_items[num_estanco]>0){

    item = aleatorio<0,num_fumadores-1>();
    
    sem_wait(puede_colocar[num_estanco]);
    mtx_output.lock();
    cout << "Estanquero " << num_estanco << "  : voy a colocar ingrediente " << item << endl;
    mtx_output.unlock();
    mostrador[num_estanco] = item;
    sem_signal(puede_consumir[num_estanco][item]);

    num_items[num_estanco]--;
  }

  sem_wait(puede_colocar[num_estanco]);
  fin[num_estanco] = true;
  for(int j = 0; j < num_fumadores; j++)
    sem_signal(puede_consumir[num_estanco][j]);
}

//-------------------------------------------------------------------------
// Función que simula la acción de fumar, como un retardo aleatoria de la hebra

void fumar(int num_estanco, int num_fumador)
{

   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_fumar( aleatorio<20,200>() );

   // informa de que comienza a fumar

   mtx_output.lock();
   cout << "Fumador " << num_estanco << "." << num_fumador << "  :"
          << " empieza a fumar (" << duracion_fumar.count() << " milisegundos)" << endl;
    mtx_output.unlock();
   // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
   this_thread::sleep_for( duracion_fumar );

   // informa de que ha terminado de fumar

   mtx_output.lock();
    cout << "Fumador " << num_estanco << "." << num_fumador << "  : termina de fumar, comienza espera de ingrediente." << endl;
    mtx_output.unlock();

}

//----------------------------------------------------------------------
// función que ejecuta la hebra del fumador
void  funcion_hebra_fumador(int num_estanco, int num_fumador)
{
  int item;
  
   while(true)
   {
     sem_wait(puede_consumir[num_estanco][num_fumador]);
     if(fin[num_estanco]) break;
     
     item = mostrador[num_estanco];
     sem_signal(puede_colocar[num_estanco]);
     fumar(num_estanco,item);
   }
}

//----------------------------------------------------------------------

int main()
{
  thread hebra_fumador[num_estancos][num_fumadores], hebra_estanquero[num_estancos];

  int i,j;

  for(i = 0; i < num_estancos; i++){
    hebra_estanquero[i]=thread(funcion_hebra_estanquero,i);
    for(j = 0; j < num_fumadores; j++)
      hebra_fumador[i][j] = thread(funcion_hebra_fumador,i,j);
  }

  for(i = 0; i < num_estancos; i++){
    hebra_estanquero[i].join();
    for(j = 0; j < num_fumadores; j++)
      hebra_fumador[i][j].join();
  }
}
