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

void fumar(int num_hebra) {
    write_mtx.lock();
    cout << "Fumador " << num_hebra << " fumando..." << endl;
    write_mtx.unlock();

    this_thread::sleep_for(chrono::milliseconds(aleatorio<20,100>()));

    write_mtx.lock();
    cout << "Fumador " << num_hebra << " ha terminado de fumar." << endl;
    write_mtx.unlock();
}

int suministrar_ingrediente() {
    int ingrediente = aleatorio<0,2>();

    write_mtx.lock();
    cout << "Estanquero deposita ingrediente " << ingrediente << " en mesa." << endl;
    write_mtx.unlock();

    return ingrediente;
}

class Estanco : public HoareMonitor {
private:
    int mesa;
    CondVar fumador[num_fumadores];
    CondVar estanquero;
public:
    Estanco();
    void ponerIngrediente(int ingrediente);
    void obtenerIngrediente(int num_fumador);
    void esperarRecogida();
};

Estanco::Estanco() {
    mesa = -1;
    for ( int i = 0; i < num_fumadores; i++ )
        fumador[i] = newCondVar();
    estanquero = newCondVar();
}

void Estanco::ponerIngrediente(int ingrediente) {
    mesa = ingrediente;
    fumador[ingrediente].signal();
}

void Estanco::obtenerIngrediente(int num_fumador) {
    if ( mesa != num_fumador )
        fumador[num_fumador].wait();
    mesa = -1;
    estanquero.signal();
}

void Estanco::esperarRecogida() {
    if ( mesa != -1 )
        estanquero.wait();
}

/**
 * @brief Hebra estanquero
 */
void hebra_estanquero(MRef<Estanco> monitor) {
    while (true) {
        int ingrediente = suministrar_ingrediente();
        monitor->ponerIngrediente(ingrediente);
        monitor->esperarRecogida();
    }
}

/**
 * @brief Hebra fumadores
 * @param monitor: referencia a monitor
 * @param num_hebra: número de hebra, 0 <= num_hebra < num_fumadores
 */
void hebra_fumadores(MRef<Estanco> monitor, int num_hebra) {
    while (true) {
        monitor->obtenerIngrediente(num_hebra);
        
        write_mtx.lock();
        cout << "Fumador " << num_hebra << " retira el producto." << endl;
        write_mtx.unlock();

        fumar(num_hebra);
    }
}

int main() {
    cout << "___________________________________________________" << endl
         << endl << "PROBLEMA DE LOS FUMADORES | MONITOR SU" << endl
         << "Alumno: Miguel Ángel Fernández Gutiérrez" << endl
         << "___________________________________________________" << endl << endl;

    MRef<Estanco> monitor = Create<Estanco>();

    thread hebra_e(hebra_estanquero, monitor);
    thread hebra_f[num_fumadores];
    for ( int i = 0; i < num_fumadores; i++ )
        hebra_f[i] = thread(hebra_fumadores, monitor, i);
    
    hebra_e.join();
    for ( int i = 0; i < num_fumadores; i++ ) hebra_f[i].join();

    return 0;
}