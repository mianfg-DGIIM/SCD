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

const int num_clientes = 4,
  num_barberos = 2; // Cada uno con su silla y su cama
 
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

void cortarPeloACliente(int j)
{
  output_mtx.lock();
  cout << "\t\t\t\t\tBarbero " << j << ": Manos a la obra!" << endl;
  output_mtx.unlock();
  
  this_thread::sleep_for(chrono::milliseconds(aleatorio<20,100>()));
  
  output_mtx.lock();
  cout << "\t\t\t\t\tBarbero " << j << ": Ya está!" << endl;
  output_mtx.unlock();
}

class Barberia : public HoareMonitor
{
private:

  CondVar sala_espera, silla_pelar[num_barberos], cama[num_barberos];

public:
  Barberia();
  void cortarPelo(int i);
  void siguienteCliente(int j);
  void finCliente(int j);
};

Barberia::Barberia()
{
  sala_espera = newCondVar();
  
  for(int j = 0; j < num_barberos; j++){
    silla_pelar[j] = newCondVar();
    cama[j] = newCondVar();
  }
}

void Barberia::cortarPelo(int i)
{

  int sitio = -1;
    
  if(sala_espera.empty())
    for(int j = 0; j < num_barberos and sitio == -1; j++)
      //if(silla_pelar[j].empty()){
      if(!cama[j].empty()){ 

	output_mtx.lock();
	cout << i << ": Que suerte! El barbero " << j << " me atiende al llegar!" << endl;
	output_mtx.unlock();
	
	sitio = j;
	cama[sitio].signal();
      }

  while(sitio == -1){

    output_mtx.lock();
    cout << i << ": Pues nada, toca esperar." << endl;
    output_mtx.unlock();

    sala_espera.wait();
    
    for(int j = 0; j < num_barberos and sitio == -1; j++){
      if(silla_pelar[j].empty()){
	sitio = j;
	output_mtx.lock();
	cout << i << ": Por fin es mi turno. Me atiende el barbero " << sitio << endl;
	output_mtx.unlock();
      }
    }
  }

  silla_pelar[sitio].wait();

  output_mtx.lock();
  cout << i << ": Muy satisfecho con mi pelado. Gracias barbero " << sitio << endl;
  output_mtx.unlock();
}

void Barberia::siguienteCliente(int j)
{
  if(sala_espera.empty() and silla_pelar[j].empty()){
    
    output_mtx.lock();
    cout << "\t\t\t\t\tBarbero " << j << ": A dormir!" << endl;
    output_mtx.unlock();
    
    cama[j].wait();
  }

  else if(silla_pelar[j].empty()){
    output_mtx.lock();
    cout << "\t\t\t\t\tBarbero " << j << ": Siguiente cliente!" << endl;
    output_mtx.unlock();
    
    sala_espera.signal();
  }
}

void Barberia::finCliente(int j)
{
  output_mtx.lock();
  cout << "\t\t\t\t\tBarbero " << j << ": Gracias por venir!" << endl;
  output_mtx.unlock();
  
  silla_pelar[j].signal();
}

void funcionHebraBarbero(MRef<Barberia> monitor, int j)
{
  while(true){
    monitor->siguienteCliente(j);
    cortarPeloACliente(j);
    monitor->finCliente(j);
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

  thread hebra_cliente[num_clientes], hebra_barbero[num_barberos];
  MRef<Barberia> monitor = Create<Barberia>();

  int i, j;

  for(j = 0; j < num_barberos; j++)
    hebra_barbero[j] = thread(funcionHebraBarbero, monitor, j);

  for(i = 0; i < num_clientes; i++)
    hebra_cliente[i] = thread(funcionHebraCliente, monitor, i);

  for(i = 0; i < num_clientes; i++)
    hebra_cliente[i].join();

  for(j = 0; j < num_barberos; j++)
    hebra_barbero[j].join();
}
