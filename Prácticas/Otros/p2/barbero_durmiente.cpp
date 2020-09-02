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

const int num_clientes = 3;
 
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

void esperarFueraBarberia(int i)
{
  this_thread::sleep_for(chrono::milliseconds(aleatorio<70,350>()));

  output_mtx.lock();
  cout << i << ": Me hace falta un pelado." << endl;
  output_mtx.unlock();
}

void cortarPeloACliente()
{
  output_mtx.lock();
  cout << "\t\t\t\t\tBarbero: Manos a la obra!" << endl;
  output_mtx.unlock();
  
  this_thread::sleep_for(chrono::milliseconds(aleatorio<20,100>()));
  
  output_mtx.lock();
  cout << "\t\t\t\t\tBarbero: Ya está!" << endl;
  output_mtx.unlock();
}

class Barberia : public HoareMonitor
{
private:

  CondVar sala_espera, silla_pelar, cama;

public:
  Barberia();
  void cortarPelo(int i);
  void siguienteCliente();
  void finCliente();
};

Barberia::Barberia()
{
  cama = newCondVar();
  silla_pelar = newCondVar();
  sala_espera = newCondVar();
}

void Barberia::cortarPelo(int i)
{
  //if(sala_espera.empty() and silla_pelar.empty()){
  if(!cama.empty()){
    output_mtx.lock();
    cout << i << ": Que suerte! La barbería vacía!" << endl;
    output_mtx.unlock();
    
    cama.signal();
  }
  else{
    output_mtx.lock();
    cout << i << ": Pues nada, toca esperar." << endl;
    output_mtx.unlock();
    
    sala_espera.wait();

    output_mtx.lock();
    cout << i << ": Por fin es mi turno." << endl;
    output_mtx.unlock();
  }
  
  silla_pelar.wait();

  output_mtx.lock();
  cout << i << ": Muy satisfecho con mi pelado." << endl;
  output_mtx.unlock();
}

void Barberia::siguienteCliente()
{
  if(sala_espera.empty() and silla_pelar.empty()){
    
    output_mtx.lock();
    cout << "\t\t\t\t\tBarbero: No hay nadie. A dormir!" << endl;
    output_mtx.unlock();
    
    cama.wait();
  }

  else if(silla_pelar.empty()){
    output_mtx.lock();
    cout << "\t\t\t\t\tBarbero: Siguiente cliente!" << endl;
    output_mtx.unlock();
    
    sala_espera.signal();
  }
}

void Barberia::finCliente()
{
  output_mtx.lock();
  cout << "\t\t\t\t\tBarbero: Gracias por venir!" << endl;
  output_mtx.unlock();
  
  silla_pelar.signal();
}

void funcionHebraBarbero(MRef<Barberia> monitor)
{
  while(true){
    monitor->siguienteCliente();
    cortarPeloACliente();
    monitor->finCliente();
  }
}

void funcionHebraCliente(MRef<Barberia> monitor, int i)
{
  while(true){
    esperarFueraBarberia(i);
    monitor->cortarPelo(i);
  }
}

int main()
{
  cout << "-------------------------------------------------------------------------------" << endl
        << "Problema del barbero durmiente (Monitor SU)." << endl
        << "-------------------------------------------------------------------------------" << endl
        << flush ;

  thread hebra_cliente[num_clientes], hebra_barbero;
  MRef<Barberia> monitor = Create<Barberia>();

  int i;

  hebra_barbero = thread(funcionHebraBarbero, monitor);

  for(i = 0; i < num_clientes; i++)
    hebra_cliente[i] = thread(funcionHebraCliente, monitor, i);

  for(i = 0; i < num_clientes; i++)
    hebra_cliente[i].join();

  hebra_barbero.join();
}
