/*
  Solución al problema de los fumadores con un monitor SU.
 */

#include <iostream>
#include <iomanip>
#include <cassert>
#include <thread>
#include <random>
#include <mutex>
#include "Semaphore.h"
#include "HoareMonitor.h"

using namespace HM;
using namespace std;

const int num_fumadores = 3;

mutex output_mtx;

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

int producirIngrediente()
{
  this_thread::sleep_for(chrono::milliseconds(aleatorio<20,100>()));

  int ingrediente = aleatorio<0,num_fumadores-1>();

  output_mtx.lock();
  cout << "Producido ingrediente " << ingrediente << endl;
  output_mtx.unlock();

  return ingrediente;
}

void fumar(int i)
{
  output_mtx.lock();
  cout << "Fumador " << i << " empieza a fumar" << endl;
  output_mtx.unlock();

  this_thread::sleep_for(chrono::milliseconds(aleatorio<20,100>()));

  output_mtx.lock();
  cout << "Fumador " << i << " termina de fumar" << endl;
  output_mtx.unlock();
}

class Estanco : public HoareMonitor
{
private:

  int mostrador;

  CondVar cola_fumador[num_fumadores], cola_estanquero;

public:
  Estanco();
  void obtenerIngrediente(int i);
  void ponerIngrediente(int ingrediente);
  void esperarRecogidaIngrediente();
};

Estanco::Estanco()
{
  mostrador = -1;
  
  for(int i = 0; i < num_fumadores; i++)
    cola_fumador[i] = newCondVar();

  cola_estanquero = newCondVar();
}

void Estanco::obtenerIngrediente(int i)
{
  if(mostrador != i)
    cola_fumador[i].wait();

  mostrador = -1;
  cola_estanquero.signal();
}

void Estanco::ponerIngrediente(int ingrediente)
{
  mostrador = ingrediente;
  cola_fumador[ingrediente].signal();
}

void Estanco::esperarRecogidaIngrediente()
{
  if(mostrador != -1)
    cola_estanquero.wait();
}

void funcionHebraEstanquero(MRef<Estanco> monitor)
{
  int ingrediente;
  
  while(true){
    ingrediente = producirIngrediente();
    monitor->ponerIngrediente(ingrediente);
    monitor->esperarRecogidaIngrediente();
  }
}

void funcionHebraFumador(MRef<Estanco> monitor, int i)
{
  while(true){
    monitor->obtenerIngrediente(i);
    fumar(i);
  }
}

int main()
{
  cout << "-------------------------------------------------------------------------------" << endl
        << "Problema de los fumadores (Monitor SU)." << endl
        << "-------------------------------------------------------------------------------" << endl
        << flush ;

  thread hebra_fumador[num_fumadores], hebra_estanquero;
  MRef<Estanco> monitor = Create<Estanco>();

  int i;

  hebra_estanquero = thread(funcionHebraEstanquero, monitor);

  for(i = 0; i < num_fumadores; i++)
    hebra_fumador[i] = thread(funcionHebraFumador, monitor, i);

  for(i = 0; i < num_fumadores; i++)
    hebra_fumador[i].join();

  hebra_estanquero.join();
}
