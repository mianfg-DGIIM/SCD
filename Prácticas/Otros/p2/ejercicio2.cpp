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

const int num_hebras = 8;

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

void esperar()
{
  this_thread::sleep_for(chrono::milliseconds(aleatorio<20,100>()));
}

class Recursos : public HoareMonitor
{
private:

  int cantidad[2];
  CondVar cola[2];

public:
  Recursos();
  void acceder(int i);
  void fin_acceso(int i);
};

Recursos::Recursos()
{
  cantidad[0] = 2; // Tipo 1
  cantidad[1] = 2; // Tipo 2
  
  cola[0] = newCondVar(); // Cola 1
  cola[1] = newCondVar(); // Cola 2
}

void Recursos::acceder(int i)
{
  int recurso = i%2;

  if(cantidad[recurso] == 0) { // Ambas unidades del recurso ocupadas
    
    output_mtx.lock();
    if(i%2) cout << "\t\t\t\t\t";
    cout << "Hebra " << i << " tiene que esperar" << endl;
    output_mtx.unlock();
    
    cola[recurso].wait();
  }

  cantidad[recurso]--; // Toma recurso

  output_mtx.lock();
  if(i%2) cout << "\t\t\t\t\t";
  cout << "Hebra " << i << " ha accedido al recurso " << i%2+1 << endl;
  output_mtx.unlock();
  
  // if(cantidad[recurso] > 0) cola[recurso].signal();
}

void Recursos::fin_acceso(int i)
{
  int recurso = i%2;
  cantidad[recurso]++; // Libera recurso

      
  output_mtx.lock();
  if(i%2) cout << "\t\t\t\t\t";
  cout << "Hebra " << i << " ha liberado el recurso " << i%2+1 << endl;
  output_mtx.unlock();
  
  cola[recurso].signal(); // Avisa de que está disponible
}

void funcionHebra(MRef<Recursos> monitor, int i)
{
  while(true){
       
    monitor->acceder(i);
    esperar();
    monitor->fin_acceso(i);
    esperar();
  }
}

int main()
{
  cout << "-------------------------------------------------------------------------------" << endl
        << "Problema de los recursos (Monitor SU)." << endl
        << "-------------------------------------------------------------------------------" << endl
        << flush ;

  thread hebra[num_hebras];
  MRef<Recursos> monitor = Create<Recursos>();

  int i;

  for(i = 0; i < num_hebras; i++)
      hebra[i] = thread(funcionHebra, monitor, i);

  for(i = 0; i < num_hebras; i++)
    hebra[i].join();
}
