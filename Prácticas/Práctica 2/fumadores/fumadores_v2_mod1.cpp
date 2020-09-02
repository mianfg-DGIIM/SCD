#include <cassert>
#include <condition_variable>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <random>
#include <thread>
#include "HoareMonitor.h"

using namespace std;
using namespace HM;

mutex write_mtx;                    // cerrojo de stdout
constexpr int num_fumadores = 3;    // número de fumadores
constexpr int num_estanqueros = 2;  // número de estanqueros

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

/**
 * @brief Función fumar
 * 
 * Realiza una espera de tiempo aleatorio
 */
void fumar(int num_fumador) {
    chrono::milliseconds time(aleatorio<20,100>());
    write_mtx.lock();
    cout << "Fumador " << num_fumador << " fumando (" << time.count() << "ms)..." << endl;
    write_mtx.unlock();

    this_thread::sleep_for(time);

    write_mtx.lock();
    cout << "Fumador " << num_fumador << " ha terminado de fumar." << endl;
    write_mtx.unlock();
}

int suministrar_ingrediente() {
    return aleatorio<0,num_ingredientes-1>();
}

class Estanco : public HoareMonitor {
private:
    int mesa;
    CondVar mostrador;
    CondVar ingrediente_disponible[num_fumadores];
public:
    Estanco();
    void ponerIngrediente(int ingrediente, int num_estanquero);
    void obtenerIngrediente(int num_fumador, int ingrediente_necesitado);
    void esperarRecogida(int num_estanquero);
};

Estanco::Estanco() {
    mesa = -1;
    for ( int i = 0; i < num_fumadores; i++ )
        ingrediente_disponible[i] = newCondVar();
    mostrador = newCondVar();
}

void Estanco::obtenerIngrediente(int num_fumador, int ingrediente_necesitado) {
    if ( mesa != ingrediente_necesitado )
        ingrediente_disponible[ingrediente_necesitado].wait();
    mesa = -1;

    write_mtx.lock();
    cout << "Fumador " << num_fumador << " retira ingrediente " << ingrediente_necesitado << endl;
    write_mtx.unlock();

    mostrador.signal();
}

void Estanco::ponerIngrediente(int ingrediente, int num_estanquero) {
    mesa = ingrediente;

    write_mtx.lock();
    cout << "Estanquero " << num_estanquero << " hace disponible ingrediente "
         << ingrediente << endl;
    write_mtx.unlock();

    ingrediente_disponible[ingrediente].signal();
}

void Estanco::esperarRecogida(int num_estanquero) {
    if ( mesa != -1 ) {
        write_mtx.lock();
        cout << "Estanquero " << num_estanquero << " espera recogida" << endl;
        write_mtx.unlock();
        mostrador.wait();
    } else {
        write_mtx.lock();
        cout << "Estanquero " << num_estanquero << " no espera recogida" << endl;
        write_mtx.unlock();
    }
}

/**
 * @brief Hebra estanquero
 */
void hebra_estanqueros(MRef<Estanco> monitor, int num_estanquero) {
    while (true) {
        int ingrediente = suministrar_ingrediente();
        monitor->ponerIngrediente(ingrediente, num_estanquero);
        monitor->esperarRecogida(num_estanquero);
    }
}

/**
 * @brief Hebra fumadores
 * @param monitor: referencia a monitor
 * @param num_fumador: número de hebra, 0 <= num_fumador < num_fumadores
 * 
 * Nótese que ingrediente_necesitado == num_fumador
 */
void hebra_fumadores(MRef<Estanco> monitor, int num_fumador, int ingrediente_necesitado) {
    while (true) {
        monitor->obtenerIngrediente(num_fumador, ingrediente_necesitado);
        fumar(num_fumador);
    }
}

int main() {
    cout << "___________________________________________________" << endl
         << endl << "PROBLEMA DE LOS FUMADORES | MONITOR SU" << endl
         << "Alumno: Miguel Ángel Fernández Gutiérrez" << endl
         << "___________________________________________________" << endl << endl;

    cout << "Número de fumadores: " << num_fumadores << endl
         << "Número de estanqueros: " << num_estanqueros << endl << endl;
    
    MRef<Estanco> monitor = Create<Estanco>();

    thread hebra_e[num_estanqueros];
    thread hebra_f[num_fumadores];
    for ( int i = 0; i < num_estanqueros; i++ )
        hebra_e[i] = thread(hebra_estanqueros, monitor, i);
    for ( int i = 0; i < num_fumadores; i++ )
        hebra_f[i] = thread(hebra_fumadores, monitor, i, i);
    
    for ( int i = 0; i < num_estanqueros; i++ ) hebra_e[i].join();
    for ( int i = 0; i < num_fumadores; i++ ) hebra_f[i].join();

    return 0;
}