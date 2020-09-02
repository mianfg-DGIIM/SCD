/**
 * @brief Exámenes práctica 1 - Ejercicio 1
 * @author Miguel Ángel Fernández Gutiérrez <mianfg@correo.ugr.es>
 *
 * Sistemas Concurrentes y Distribuidos
 * 3º Doble Grado en Ingeniería Informática y Matemáticas, 2019/2020
 */
#include <cassert>
#include <chrono>  // duraciones (duration), unidades de tiempo
#include <iostream>
#include <mutex>
#include <random>  // dispositivos, generadores y distribuciones aleatorias
#include <thread>
#include "Semaphore.h"

using namespace std;
using namespace SEM;

const int num_fumadores = 3;  // número de fumadores
int mostrador = -1;           // el mostrador está inicialmente vacío
int provee = -1;              // el proveedor inicialmente no provee nada
Semaphore estanquero_a_mostrador(1), fumador_de_mostrador[num_fumadores] = {0},
                                     nuevo_ingrediente_de_proveedor(1),
                                     estanquero_de_proveedor(0);

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
 * @note EXAMEN
 * @brief Función que ejecuta la hebra del proveedor
 */
void funcion_hebra_proveedor() {
    while (true) {
        nuevo_ingrediente_de_proveedor.sem_wait();
        provee = aleatorio<0, num_fumadores - 1>();
        cout << "Proveedor provee el ingrediente: " << provee << endl;
        estanquero_de_proveedor.sem_signal();
    }
}
/**
 * @brief Función que ejecuta la hebra del estanquero
 */
void funcion_hebra_estanquero() {
    while (true) {
        estanquero_de_proveedor.sem_wait();
        mostrador = provee;
        cout << "Estanquero ha tomado ingrediente proveído: " << mostrador
             << endl;
        estanquero_a_mostrador.sem_wait();
        // mostrador = aleatorio<0, num_fumadores-1>();
        cout << "Estanquero ha depositado ingrediente: " << mostrador << endl;
        fumador_de_mostrador[mostrador].sem_signal();
        nuevo_ingrediente_de_proveedor.sem_signal();
    }
}

/**
 * @brief Simula la acción de fumar como un retardo aleatorio de la hebra
 * @param num_fumador: número del fumador, 0 <= num_fumador <= num_fumadores-1
 */
void fumar(int num_fumador) {
    // calcular milisegundos aleatorios de duración de la acción de fumar)
    chrono::milliseconds duracion_fumar(aleatorio<20, 200>());

    // informa de que comienza a fumar
    cout << "Fumador " << num_fumador << ":"
         << " empieza a fumar (" << duracion_fumar.count() << " milisegundos)"
         << endl;

    // espera bloqueada un tiempo igual a 'duracion_fumar' milisegundos
    this_thread::sleep_for(duracion_fumar);

    // informa de que ha terminado de fumar
    cout << "Fumador " << num_fumador << ": termina de fumar, comienza espera "
         << "de ingrediente." << endl;
}

/**
 * @brief Fución que ejecuta la hebra del fumador
 * @param num_fumador: número del fumador, 0 <= num_fumador <= num_fumadores-1
 */
void funcion_hebra_fumador(int num_fumador) {
    while (true) {
        fumador_de_mostrador[num_fumador].sem_wait();
        // if (mostrador == num_fumador)
        //   mostrador = -1;
        fumar(num_fumador);
        estanquero_a_mostrador.sem_signal();
    }
}

int main() {
    // declarar hebras y ponerlas en marcha
    thread hebra_proveedor(funcion_hebra_proveedor);
    thread hebra_estanquero(funcion_hebra_estanquero);
    thread hebra_fumador[num_fumadores];

    for (int i = 0; i < num_fumadores; i++)
        hebra_fumador[i] = thread(funcion_hebra_fumador, i);

    hebra_proveedor.join();
    hebra_estanquero.join();
    for (int i = 0; i < num_fumadores; i++) hebra_fumador[i].join();
}
