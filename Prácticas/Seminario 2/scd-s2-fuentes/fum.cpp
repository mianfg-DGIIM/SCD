// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Práctica 2. Casos prácticos de monitores en C++11.
// Antonio Coín Castro.
//
// Archivo: fumadores_SU.cpp
//
// Monitor en C++11 con semántica SU, para el problema
// de los fumadores. Se contemplan las siguientes modificaciones:
//
//    * Hay un número variable de fumadores.
//    * Hay varios estanqueros, pero un solo mostrador.
//
// -----------------------------------------------------------------------------

#include <iostream>
#include <cassert>
#include <thread>
#include <random>
#include <mutex>
#include "HoareMonitor.h"

using namespace std;
using namespace HM;


//**********************************************************************
// Variables globales
//----------------------------------------------------------------------

constexpr int NF = 3,  // Número de fumadores
              NE = 2;  // Número de estanqueros

mutex mtx;             // Exclusión mutua para la salida por pantalla


//**********************************************************************
// Funciones auxiliares
//----------------------------------------------------------------------

// Generar número pseudoaleatorio en el rango [min,max]
template<int min, int max> int aleatorio()
{
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme(min, max);
  return distribucion_uniforme(generador);
}

//----------------------------------------------------------------------

// Simular la acción de fumar
void fumar(int num_fumador)
{
  chrono::milliseconds duracion_fumar( aleatorio<20,200>() );

  mtx.lock();
  cout << "\t\tFumador " << num_fumador << ": empieza a fumar ("
       << duracion_fumar.count() << " milisegundos)..." << endl;
  mtx.unlock();

  this_thread::sleep_for(duracion_fumar);

  mtx.lock();
  cout << "\t\tFumador " << num_fumador << ": termina de fumar, comienza espera de ingrediente "
       << num_fumador << "." << endl;
  mtx.unlock();
}

//----------------------------------------------------------------------

// Simular la acción de poner un ingrediente en el mostrador
int producirIngrediente()
{
  this_thread::sleep_for(chrono::milliseconds(aleatorio<100,400>() ));
  return aleatorio<0,NF-1>();  // Devuelve ingrediente producido
}


//**********************************************************************
// Monitor para gestionar el mostrador
//----------------------------------------------------------------------

class Estanco : public HoareMonitor {
  private:
    int ingrediente;   // -1 == no hay ingrediente
    CondVar mostr_vacio;
    CondVar ingr_disp[NF];
  public:
    Estanco();
    void obtenerIngrediente(int i);
    void ponerIngrediente(int i, int num_estanquero);
    void esperarRecogidaIngrediente();
};


//**********************************************************************
// Implementación de métodos del monitor
//----------------------------------------------------------------------

Estanco::Estanco()
{
  ingrediente = -1;
  mostr_vacio = newCondVar();

  for (int i = 0; i < NF; i++)
    ingr_disp[i] = newCondVar();
}

//----------------------------------------------------------------------

void Estanco::obtenerIngrediente(int i)
{
  if (ingrediente != i)
    ingr_disp[i].wait();

  ingrediente = -1;

  mtx.lock();
  cout << "\t\tRetirado ingrediente: " << i << endl;
  mtx.unlock();

  mostr_vacio.signal();
}

//----------------------------------------------------------------------

void Estanco::ponerIngrediente(int i, int num_estanquero)
{
  ingrediente = i;

  mtx.lock();
  cout << "Estanquero " << num_estanquero << ": disponible ingrediente "
       << ingrediente << endl;
  mtx.unlock();

  ingr_disp[i].signal();
}

//----------------------------------------------------------------------

void Estanco::esperarRecogidaIngrediente()
{
  if (ingrediente != -1)
    mostr_vacio.wait();
}


// *****************************************************************************
// Funciones de hebras
// -----------------------------------------------------------------------------

void funcion_hebra_estanquero(MRef<Estanco> monitor, int i)
{
   while(true)
   {
     int ingrediente = producirIngrediente();
     monitor->ponerIngrediente(ingrediente, i);
     monitor->esperarRecogidaIngrediente();
   }
}

// -----------------------------------------------------------------------------

void funcion_hebra_fumador(MRef<Estanco> monitor, int i)
{
  while(true)
  {
    monitor->obtenerIngrediente(i);
    fumar(i);
  }
}


// *****************************************************************************
// Programa principal
// -----------------------------------------------------------------------------

int main()
{
  int i;

  cout << endl << "--------------------------------------------------------" << endl
               << "Problema de los fumadores" << endl
               << "--------------------------------------------------------" << endl
               << endl << flush ;

  cout << "NÚMERO DE FUMADORES: " << NF << endl;
  cout << "NÚMERO DE ESTANQUEROS: " << NE << endl << endl << flush;

  auto monitor = Create<Estanco>();

  thread hebra_estanquero[NE],
         hebra_fumador[NF];

  for(i = 0; i < NE; i++)
    hebra_estanquero[i] = thread(funcion_hebra_estanquero, monitor, i);
  for(i = 0; i < NF; i++)
    hebra_fumador[i] = thread(funcion_hebra_fumador, monitor, i);

  // Hay que esperar al menos a una hebra
  hebra_estanquero[0].join();
}