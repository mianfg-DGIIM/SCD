#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random>
#include "Semaphore.h"

using namespace std ;
using namespace SEM ;

const unsigned int num_items = 40 ,   
	       tam_vec   = 10 ; 
int product1 = 0, product2 = 0;  
unsigned  cont_prod[num_items] = {0}, 
          cont_cons[num_items] = {0}; 

Semaphore productor1(tam_vec);
Semaphore productor2(tam_vec);
Semaphore consumidor(0);
Semaphore interno(1);
Semaphore imprimir(0);

int consumidos = 0;

int vec [tam_vec];
int pp = 0;


//----------------------------------------------------------------------

template< int min, int max > int aleatorio()
{
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
  return distribucion_uniforme( generador );
}

//----------------------------------------------------------------------

int producir_dato()
{
   static int contador = 0 ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));

   cout << "\tProducido: " << contador << endl << flush ;

   cont_prod[contador] ++ ;
   return contador++ ;
}
//----------------------------------------------------------------------

void consumir_dato( unsigned dato )
{
   assert( dato < num_items );
   cont_cons[dato] ++ ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));

   cout << "\tConsumido: " << dato << endl ;
   
}


//----------------------------------------------------------------------

void test_contadores()
{
   bool ok = true ;
   cout << "\n\nComprobando contadores ... \n\n" ;
   for( unsigned i = 0 ; i < num_items ; i++ )
   {  if ( cont_prod[i] != 1 )
      {  cout << "ERROR: \tValor " << i << " producido " << cont_prod[i] << " veces." << endl ;
         ok = false ;
      }
      if ( cont_cons[i] != 1 )
      {  cout << "ERROR: \tValor " << i << " consumido " << cont_cons[i] << " veces" << endl ;
         ok = false ;
      }
   }
   if (ok)
      cout << endl << flush << "SOLUCION (aparentemente) CORRECTA." << endl << flush ;
}

//----------------------------------------------------------------------

void  funcion_hebra_productora1(  )
{
   for( unsigned i = 0 ; i < num_items/2 ; i++ )
   {
	cout << "PRODUCIENDO..."<< endl;
	
	product1++;
	int dato = producir_dato() ;

      	sem_wait(productor1);
	sem_wait(interno);

      	vec[pp] = dato;
	pp++;

	sem_signal(interno);
      	sem_signal(consumidor);
	sem_signal(productor2);
      
   }
   cout << "----------------------------------------------------------------------------" << endl;
   cout << "\n-->El numero total de articulos suministrados por el primer proveedor es: " << product1 << endl << endl;
   cout << "----------------------------------------------------------------------------" << endl;
}

//----------------------------------------------------------------------

void  funcion_hebra_productora2(  )
{
   for( unsigned i = 0 ; i < num_items/2 ; i++ )
   {
	cout << "PRODUCIENDO..."<< endl;
	
 	product2++;	
	int dato = producir_dato() ;


      	sem_wait(productor2);
	sem_wait(interno);

      	vec[pp] = dato;
	pp++;

	sem_signal(interno);
      	sem_signal(consumidor);
	sem_signal(productor1);
      
   }
   cout << "----------------------------------------------------------------------------" << endl;
   cout << "\n-->El numero total de articulos suministrados por el segundo proveedor es: " << product2 << endl << endl;
   cout << "----------------------------------------------------------------------------" << endl;
}

//----------------------------------------------------------------------

void funcion_hebra_consumidora(  )
{
   for( unsigned i = 0 ; i < num_items ; i++ )
   {
      	consumidos ++;
	if (consumidos%5 == 0){
		sem_signal(imprimir);
	}
	
	int dato ;

      	sem_wait(consumidor);
      	sem_wait(interno);
	
	cout << "CONSUMIENDO..."<< endl;

	pp--;
      	dato = vec[pp]; 

      	consumir_dato( dato ) ;

	sem_signal(interno);

    }
}

//----------------------------------------------------------------------
void funcion_hebra_imprimir( )
{
	for(int i = 0; i < num_items / 5; i++){
		sem_wait(imprimir);
		cout << "-------------------------------------------" << endl;
		cout << "IMPRIMIENDO..."<< endl;
		cout << "\tSe han consumido un pack de 5 items" << endl;
		cout << "-------------------------------------------" << endl;
	}
	
}
//----------------------------------------------------------------------

int main()
{
   cout << "--------------------------------------------------------" << endl
        << "Problema de los productores-consumidores (solucion LIFO)." << endl
        << "--------------------------------------------------------" << endl
        << flush ;

   thread hebra_productora1 ( funcion_hebra_productora1 ),
	  hebra_productora2 ( funcion_hebra_productora2 ),
	  hebra_imprimir ( funcion_hebra_imprimir ),
          hebra_consumidora( funcion_hebra_consumidora );

   hebra_productora1.join() ;
   hebra_productora2.join() ;
   hebra_consumidora.join() ;
   hebra_imprimir.join();

   test_contadores();
}

