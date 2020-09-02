/**
 * @brief Fumadores con monitores SU
 * @author Miguel Ángel Fernández Gutiérrez <mianfg@correo.ugr.es>
 */
#include <cassert>
#include <iostream>
#include <mutex>
#include <random>
#include <thread>
#include "HoareMonitor.h"

using namespace std;
using namespace HM;

mutex write_mtx;                  // cerrojo de stdout
constexpr int num_fumadores = 6,  // número de fumadores
    num_estanqueros = 2,          // número de estanqueros
    num_ingredientes = 3;         // número de ingredientes

/**
 * @brief Genera un entero aleatorio uniformemente distribuido entre dos valores
 * enteros, ambos incluidos.
 * @param min: primer valor
 * @param max: último valor
 * @note Ambos tienen que ser dos constantes, conocidas en tiempo de compilación
 */
template <int min, int max>
int aleatorio() {
    static default_random_engine generador((random_device())());
    static uniform_int_distribution<int> distribucion_uniforme(min, max);
    return distribucion_uniforme(generador);
}
//----------------------------------------------------------------------

/**
 * @brief Función fumar
 *
 * Realiza una espera de tiempo aleatorio
 */
void fumar(int num_fumador) {
    chrono::milliseconds time(aleatorio<20, 200>());

    write_mtx.lock();
    cout << "\t\tFumador " << num_fumador << ": fumando (" << time.count()
         << "ms)..." << endl;
    write_mtx.unlock();

    this_thread::sleep_for(time);

    write_mtx.lock();
    cout << "\t\tFumador " << num_fumador
         << ": termina de fumar, comienza espera de ingrediente" << endl;
    write_mtx.unlock();
}

/**
 * @brief Función suministrar ingrediente
 *
 * Realiza una espera de tiempo aleatorio
 */
int suministrarIngrediente() {
    this_thread::sleep_for(chrono::milliseconds(aleatorio<100, 400>()));
    return aleatorio<-1, num_ingredientes - 1>();
    // el ingrediente -1 es en este caso el tipo especial: hora de cerrar el estanco
}

/**
 * @brief Clase monitor para estanco
 *
 * @author Miguel Ángel Fernández Gutiérrez
 */
class Estanco : public HoareMonitor {
   private:
    int mesa;
    CondVar mostrador_vacio;
    CondVar ingrediente_disponible[num_fumadores];

   public:
    Estanco();
    bool obtenerIngrediente(int num_fumador, int ingrediente_necesitado);
    void ponerIngrediente(int ingrediente, int num_estanquero);
    bool esperarRecogida(int num_estanquero);
};

Estanco::Estanco() {
    mesa = -2;   // inicialmente no hay nada en el mostrador
    mostrador_vacio = newCondVar();

    for (int i = 0; i < num_fumadores; i++) ingrediente_disponible[i] = newCondVar();
}

bool Estanco::obtenerIngrediente(int num_fumador, int ingrediente_necesitado) {
    bool continuar = true;

    if (mesa != ingrediente_necesitado)
        ingrediente_disponible[ingrediente_necesitado].wait();
    
    if (mesa == -1)
        continuar = false;
    else {
        mesa = -2;

        write_mtx.lock();
        cout << "\t\tFumador " << num_fumador << " retira ingrediente: " << ingrediente_necesitado << endl;
        write_mtx.unlock();

        
    }

    mostrador_vacio.signal();

    return continuar;
}

void Estanco::ponerIngrediente(int ingrediente, int num_estanquero) {
    if ( ingrediente != -1 ) {
        mesa = ingrediente;

        write_mtx.lock();
        cout << "Estanquero " << num_estanquero << ": disponible ingrediente "
            << mesa << endl;
        write_mtx.unlock();

        if ( ingrediente == -1 ) {
            for ( int i = 0; i < num_ingredientes; i++ )
                ingrediente_disponible[ingrediente].signal();
        } else {
            ingrediente_disponible[ingrediente].signal();
        }
    } else {
        write_mtx.lock();
        cout << "Estanquero ha encontrado ingrediente " << num_estanquero << endl;
        write_mtx.unlock();
    }
}

bool Estanco::esperarRecogida(int num_estanquero) {
    bool continuar = true;
    
    mostrador_vacio.wait();

    if (mesa != -2)
        continuar = true;
    else if (mesa == -1) {
        continuar = false;
    }

    return continuar;
}

/**
 * @brief Hebra estanquero
 */
void hebra_estanqueros(MRef<Estanco> monitor, int num_estanquero) {
    bool continuar = true;
    while (continuar) {
        int ingrediente = suministrarIngrediente();
        monitor->ponerIngrediente(ingrediente, num_estanquero);
        continuar = monitor->esperarRecogida(num_estanquero);
    }
}

/**
 * @brief Hebra fumador
 */
void hebra_fumadores(MRef<Estanco> monitor, int num_fumador, int ingrediente_necesitado) {
    write_mtx.lock();
    cout << "Fumador " << num_fumador << " necesita " << ingrediente_necesitado << endl;
    write_mtx.unlock();
    bool continuar = true;
    while (continuar) {
        continuar = monitor->obtenerIngrediente(num_fumador, ingrediente_necesitado);
        fumar(num_fumador);
    }
}

// *****************************************************************************
// Programa principal
// -----------------------------------------------------------------------------

int main() {
    cout << "___________________________________________________" << endl
         << endl << "PROBLEMA DE LOS FUMADORES | MONITOR SU" << endl
         << "Alumno: Miguel Ángel Fernández Gutiérrez" << endl
         << "___________________________________________________" << endl << endl;

    cout << "Número de fumadores: " << num_fumadores << endl
         << "Número de estanqueros: " << num_estanqueros << endl << endl
         << flush;

    MRef<Estanco> monitor = Create<Estanco>();

    thread hebra_e[num_estanqueros];
    thread hebra_f[num_fumadores];
    for ( int i = 0; i < num_estanqueros; i++ )
        hebra_e[i] = thread(hebra_estanqueros, monitor, i);
    int ingr = 0;
    for ( int i = 0; i < num_fumadores; i++ ) {
        hebra_f[i] = thread(hebra_fumadores, monitor, i, ingr);
        ingr = (ingr+1)%num_ingredientes;
    }
    cout << endl;
    for ( int i = 0; i < num_estanqueros; i++ ) hebra_e[i].join();
    for ( int i = 0; i < num_fumadores; i++ ) hebra_f[i].join();
}