/*
  Solución al problema de los fumadores con un monitor SU.
 */

// David Cabezas Berrido

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

const int num_clientes = 10;
const int n_sillas = 5; // Capacidad de la sala de espera

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
{                                    // 500, 2500 para que el barbero duerma
  this_thread::sleep_for(chrono::milliseconds(aleatorio<100,500>()));

  output_mtx.lock();
  cout << i << ": Me hace falta un pelado." << endl;
  output_mtx.unlock();
}

void cortarPeloACliente()
{
  output_mtx.lock();
  cout << "\t\t\t\t\tBarbero: Manos a la obra!" << endl;
  output_mtx.unlock();
  
  this_thread::sleep_for(chrono::milliseconds(aleatorio<30,150>()));
  
  output_mtx.lock();
  cout << "\t\t\t\t\tBarbero: Ya está!" << endl;
  output_mtx.unlock();
}

class Barberia : public HoareMonitor
{
private:

  CondVar sala_espera, silla_pelar, cama, puerta;
  
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
  puerta = newCondVar();
}

void Barberia::cortarPelo(int i)
{
  if(sala_espera.get_nwt() >= n_sillas){ // La sala de espera está llena
    
    output_mtx.lock();
    cout << i << ": Vaya, me toca esperar en la puerta." << endl;
    output_mtx.unlock();

    puerta.wait();

    output_mtx.lock();
    cout << i << ": Ya puedo entrar en la barbería." << endl;
    output_mtx.unlock();
  }
    
  //if(sala_espera.empty() and silla_pelar.empty()){
  if(!cama.empty()){
    output_mtx.lock();
    cout << i << ": He encontrado al barbero durmiendo." << endl;
    output_mtx.unlock();

    if(!sala_espera.empty()){

      output_mtx.lock();
      cout << i << ": Despierto al barbero." << endl;
      output_mtx.unlock();
      
      cama.signal();
    }
    else{
      output_mtx.lock();
      cout << i << ": Ya lo despertará otro." << endl;
      output_mtx.unlock();
    }
  }

  output_mtx.lock();
  cout << i << ": Voy a esperar en la sala de espera." << endl;
  output_mtx.unlock();
  
  sala_espera.wait();

  output_mtx.lock();
  cout << i << ": Por fin es mi turno. Avisaré a los de la puerta." << endl;
  output_mtx.unlock();

  puerta.signal();

  output_mtx.lock();
  cout << i << ": Me siento en la silla de pelar." << endl;
  output_mtx.unlock();
  
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

    output_mtx.lock();
    cout << "\t\t\t\t\tBarbero: Me han despertado!" << endl;
    output_mtx.unlock();
  }

  output_mtx.lock();
  cout << "\t\t\t\t\tBarbero: Siguiente cliente!" << endl;
  output_mtx.unlock();
    
  sala_espera.signal();
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
