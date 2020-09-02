/**
 * @brief Exámenes práctica 1 - Ejercicio 2
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

const int num_fumadores = 3,  // número de fumadores
    num_estanqueros = 2;      // número de estanqueros
int mostrador = -1;           // el mostrador está inicialmente vacío
Semaphore estanquero_a_mostrador[num_estanqueros] = {0, 0},
    fumador_de_mostrador[num_fumadores] = {0, 0, 0},
    proveer(1);

int ingrediente_anterior[num_estanqueros] = {-1, -1};

/**
 * @note NOTA
 * A pesar de declarar arrays de semáforos de tamaño variable, la implementación
 * del semáforo de Semaphore no permite inicializar un array de semáforos.
 * Por eso insertamos el array manualmente; una sencilla modificación de
 * Semaphore permitirá este comportamiento.
 */

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
 * @brief Función que ejecuta la hebra del estanquero
 */
void funcion_hebra_estanquero(int num_estanquero) {
    while (true) {
        estanquero_a_mostrador[num_estanquero].sem_wait();
        mostrador = aleatorio<0, num_fumadores - 1>();
        cout << "Estanquero " << num_estanquero
             << " ha depositado ingrediente: " << mostrador << endl;
        if (mostrador == ingrediente_anterior[num_estanquero])
            cout << "--Estanquero " << num_estanquero
                 << " ha producido dos veces seguidas el ingrediente "
                 << mostrador << endl;
        ingrediente_anterior[num_estanquero] = mostrador;
        fumador_de_mostrador[mostrador].sem_signal();
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
        proveer.sem_signal();
    }
}

int main() {
    // declarar hebras y ponerlas en marcha
    thread hebra_estanquero[num_estanqueros];
    thread hebra_fumador[num_fumadores];

    for (int i = 0; i < num_estanqueros; i++)
        hebra_estanquero[i] = thread(funcion_hebra_estanquero, i);
    for (int i = 0; i < num_fumadores; i++)
        hebra_fumador[i] = thread(funcion_hebra_fumador, i);

    int turno_estanquero = -1;
    while (true) {
        proveer.sem_wait();
        turno_estanquero = aleatorio<0, num_estanqueros - 1>();
        estanquero_a_mostrador[turno_estanquero].sem_signal();
    }

    for (int i = 0; i < num_estanqueros; i++) hebra_estanquero[i].join();
    for (int i = 0; i < num_fumadores; i++) hebra_fumador[i].join();
}
