// -----------------------------------------------------------------------------
// Sistemas concurrentes y Distribuidos.
// Seminario 1. Programación Multihebra y Semáforos.
//
// Ejemplo 9 (ejemplo9.cpp)
// Calculo concurrente de una integral. Plantilla para completar.
//
// Historial:
// Creado en Abril de 2017
// -----------------------------------------------------------------------------
// David Cabezas Berrido

#include <iostream>
#include <iomanip>
#include <chrono>  // incluye now, time\_point, duration
#include <future>
#include <vector>
#include <cmath>

using namespace std ;
using namespace std::chrono;

const long m  = 10*1024l*1024l, //intervalos
  n = 8; //nthreads


// -----------------------------------------------------------------------------
// evalua la función $f$ a integrar ($f(x)=4/(1+x^2)$)
double f( double x )
{
  return 4.0/(1.0+x*x) ;
}
// -----------------------------------------------------------------------------
// calcula la integral de forma secuencial, devuelve resultado:
double calcular_integral_secuencial(  )
{
   double suma = 0.0 ;                        // inicializar suma
   for( long i = 0 ; i < m ; i++ )            // para cada $i$ entre $0$ y $m-1$:
      suma += f( (i+double(0.5)) /m );         //   $~$ añadir $f(x_i)$ a la suma actual
   return suma/m ;                            // devolver valor promedio de $f$
}

// -----------------------------------------------------------------------------
// función que ejecuta cada hebra: recibe $i$ ==índice de la hebra, ($0\leq i<n$)

double funcion_hebra_contigua( long i ) //Asignación contigua
{
  long chunk = ceil(m/n);
  long top = (i+1)*chunk;
  if(i == n-1) top = m;
  
  double suma = 0.0;
  for(long j = i*chunk; j < top; j++)
    suma += f( (j+double(0.5))/m );
  
  return suma;
}

double funcion_hebra_entrelazada( long i ) //Asignación entrelazada
{
  double suma = 0.0;
  for(long j = i; j < m; j+=n)
    suma += f( (j+double(0.5))/m );

  return suma;
}

// -----------------------------------------------------------------------------
// calculo de la integral de forma concurrente con asignación de hebras contigua
double calcular_integral_concurrente_contigua( )
{
  future<double> futuros[n];

  for(int i = 0; i < n; i++)
    futuros[i] = async(launch::async, funcion_hebra_contigua,i);

  double suma = 0.0;
  for(int i = 0; i < n; i++)
    suma += futuros[i].get();
  
  return suma/m;
}

// calculo de la integral de forma concurrente con asignación de hebras entrelazada
double calcular_integral_concurrente_entrelazada( )
{
  future<double> futuros[n];

  for(int i = 0; i < n; i++)
    futuros[i] = async(launch::async, funcion_hebra_entrelazada,i);

  double suma = 0.0;
  for(int i = 0; i < n; i++)
    suma += futuros[i].get();
  
  return suma/m;
}
// -----------------------------------------------------------------------------

int main()
{

  time_point<steady_clock> inicio_sec  = steady_clock::now() ;
  const double             result_sec  = calcular_integral_secuencial(  );
  time_point<steady_clock> fin_sec     = steady_clock::now() ;
  
  time_point<steady_clock> inicio_conc_cont = steady_clock::now() ;
  const double             result_conc_cont = calcular_integral_concurrente_contigua(  );
  time_point<steady_clock> fin_conc_cont    = steady_clock::now() ;
  
  time_point<steady_clock> inicio_conc_entr = steady_clock::now() ;
  const double             result_conc_entr = calcular_integral_concurrente_entrelazada(  );
  time_point<steady_clock> fin_conc_entr    = steady_clock::now() ;
  
  duration<float,milli>    tiempo_sec  = fin_sec  - inicio_sec ,
                           tiempo_conc_cont = fin_conc_cont - inicio_conc_cont ,
                           tiempo_conc_entr = fin_conc_entr - inicio_conc_entr;
  const float              porc_cont        = 100.0*tiempo_conc_cont.count()/tiempo_sec.count(),
                           porc_entr        = 100.0*tiempo_conc_entr.count()/tiempo_sec.count();


  constexpr double pi = 3.14159265358979323846l ;

  cout << "Número de muestras (m)   : " << m << endl
       << "Número de hebras (n)     : " << n << endl
       << setprecision(18)
       << "Valor de PI                                    : " << pi << endl
       << "Resultado secuencial                           : " << result_sec  << endl
       << "Resultado concurrente (asignación contigua)    : " << result_conc_cont << endl
       << "Resultado concurrente (asignación entrelazada) : " << result_conc_entr << endl
       << setprecision(5)
       << "Tiempo secuencial        : " << tiempo_sec.count()  << " milisegundos. " << endl
       << "Tiempo concurrente (asignación contigua) : " << tiempo_conc_cont.count() << " milisegundos. " << endl
       << "Tiempo concurrente (asignación entrelazada) : " << tiempo_conc_entr.count() << " milisegundos. " << endl
    
       << setprecision(4)
       << "Porcentaje t.conc_cont/t.sec. : " << porc_cont << "%" << endl
       << "Porcentaje t.conc_entr/t.sec. : " << porc_entr << "%" << endl;
}
