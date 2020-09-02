#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> 
#include <chrono> 
#include "Semaphore.h"

using namespace std ;
using namespace SEM ;

Semaphore fumador[2]={0,0};
Semaphore mostrador(1);
Semaphore proveedor(1);
Semaphore estanquero(0);
int producto, cigarrillos = 0;
int a = 0, b = 0;


//----------------------------------------------------------------------

template< int min, int max > int aleatorio()
{
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
  return distribucion_uniforme( generador );
}

//----------------------------------------------------------------------
void funcion_hebra_proveedor()
{
	while (true){
		sem_wait(proveedor);

		cout << "Abasteciendo... " << endl;

		producto = aleatorio<0,1>();

		cout <<"\t\t-->Producto " << producto << " creado. " << endl;

		sem_signal(estanquero);
	}
}
//----------------------------------------------------------------------

void funcion_hebra_estanquero()
{
	while (true){
		sem_wait(estanquero);	

		cout << "Suministrando... " << endl;

		sem_wait(mostrador);

		cout <<"\t\t-->Producto " << producto << " disponible. " << endl;

		sem_signal( fumador[producto] );

		sem_signal(proveedor);
	}
}

//-------------------------------------------------------------------------


void fumar( int num_fumador )
{
    	chrono::milliseconds duracion_fumar( aleatorio<20,200>() );
    	
	cout << "\nFumando..." << endl;
	cout << "\tEl fumador " << num_fumador << "  :"
          << " empieza a fumar durante (" << duracion_fumar.count() << " milisegundos)" << endl;

    	this_thread::sleep_for( duracion_fumar );

    	cout << "\tEl fumador " << num_fumador << "  : termina de fumar, comienza espera de ingrediente." << endl << endl;

	if(num_fumador ==  0){
			a++;
			cigarrillos = a;
	}else{
			b++;
			cigarrillos = b;
	}
		
	cout << "\tEL FUMADOR "<< num_fumador << " HA FUMADO " << cigarrillos << " VECES." << endl;

}

//----------------------------------------------------------------------

void  funcion_hebra_fumador( int num_fumador )
{
	while( true )
   	{	
		sem_wait(fumador[num_fumador]);

		cout <<"\t\t-->Producto " << producto << " retirado. " << endl; 

		
		
		sem_signal(mostrador);

		fumar(num_fumador);
		
		cigarrillos = a + b;
		cout << "\tEN TOTAL, HAN FUMADO "<< cigarrillos << " VECES." << endl;		
   	}
}

//----------------------------------------------------------------------

int main()
{
   	int fum1  = 0, fum2 = 1;
	thread  hebra_proveedor( funcion_hebra_proveedor ),
		hebra_estanquero ( funcion_hebra_estanquero ),
          	hebra_fumador1( funcion_hebra_fumador, fum1 ),
		hebra_fumador2( funcion_hebra_fumador, fum2 );

   	hebra_proveedor.join() ;
	hebra_estanquero.join() ;
   	hebra_fumador1.join() ;
	hebra_fumador2.join() ;
}
