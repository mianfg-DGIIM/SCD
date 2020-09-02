/**
 * @brief Problema del productor-consumidor - Implementación multihebra
 * @author Miguel Ángel Fernández Gutiérrez <mianfg@correo.ugr.es>
 *
 * Sistemas Concurrentes y Distribuidos
 * 3º Doble Grado en Ingeniería Informática y Matemáticas, 2019/2020
 */
#include <atomic>
#include <cassert>
#include <iostream>
#include <mutex>
#include <random>
#include <thread>
#include "Semaphore.h"

using namespace std;
using namespace SEM;

// ** VARIABLES COMPARTIDAS **

const int num_items = 40,             // número de items
    tam_vec = 5;                      // tamaño del buffer
unsigned cont_prod[num_items] = {0},  // contadores de verificación: producidos
    cont_cons[num_items] = {0};       // contadores de verificación: consumidos
const int num_productores = 3,        // número de productores
    num_consumidores = 3;             // número de consumidores

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

// ** FUNCIONES COMUNES (versiones FIFO y LIFO) **

/**
 * @brief Produce un dato
 * @param num_hebra: índice de la hebra
 * @return El dato producido
 */
int producir_dato(int num_hebra) {
    static int contador = 0;
    this_thread::sleep_for(chrono::milliseconds(aleatorio<20, 100>()));

    cout << "producido: " << contador << " (hebra " << num_hebra << ")" << endl
         << flush;

    cont_prod[contador]++;
    return contador++;
}

/**
 * @brief Consume un dato
 * @param dato: dato que consume
 * @param num_hebra: índice de la hebra
 */
void consumir_dato(unsigned dato, int num_hebra) {
    assert(dato < num_items);
    cont_cons[dato]++;
    this_thread::sleep_for(chrono::milliseconds(aleatorio<20, 100>()));

    cout << "                  consumido: " << dato << " (hebra " << num_hebra
         << ")" << endl;
}

/**
 * @brief Comprobador de contadores
 */
void test_contadores() {
    bool ok = true;
    cout << "comprobando contadores ....";
    for (unsigned i = 0; i < num_items; i++) {
        if (cont_prod[i] != 1) {
            cout << "error: valor " << i << " producido " << cont_prod[i]
                 << " veces." << endl;
            ok = false;
        }
        if (cont_cons[i] != 1) {
            cout << "error: valor " << i << " consumido " << cont_cons[i]
                 << " veces" << endl;
            ok = false;
        }
    }
    if (ok)
        cout << endl
             << flush << "solución (aparentemente) correcta." << endl
             << flush;
}

//------------------------------------------------------------------------------

// ** SOLUCIÓN LIFO **

Semaphore puede_leer_LIFO(0), puede_escribir_LIFO(tam_vec);
int buffer_LIFO[tam_vec];
atomic<int> loc_LIFO;

void funcion_hebra_productora_LIFO(int num_hebra) {
    for (unsigned i = num_hebra; i < num_items; i += num_productores) {
        int dato = producir_dato(num_hebra);
        puede_escribir_LIFO.sem_wait();

        buffer_LIFO[loc_LIFO++] = dato;

        puede_leer_LIFO.sem_signal();
    }
}

void funcion_hebra_consumidora_LIFO(int num_hebra) {
    for (unsigned i = num_hebra; i < num_items; i += num_consumidores) {
        int dato;
        puede_leer_LIFO.sem_wait();

        dato = buffer_LIFO[--loc_LIFO];

        puede_escribir_LIFO.sem_signal();
        consumir_dato(dato, num_hebra);
    }
}

// ** SOLUCIÓN FIFO **

Semaphore puede_leer_FIFO(0), puede_escribir_FIFO(tam_vec);
int buffer_FIFO[tam_vec];
atomic<int> leer_FIFO, escribir_FIFO;

void funcion_hebra_productora_FIFO(int num_hebra) {
    int prox;
    for (unsigned i = num_hebra; i < num_items; i += num_productores) {
        int dato = producir_dato(num_hebra);
        puede_escribir_FIFO.sem_wait();

        buffer_FIFO[escribir_FIFO] = dato;
        prox = (escribir_FIFO + 1) % tam_vec;
        escribir_FIFO.store(prox, memory_order::memory_order_acquire);

        puede_leer_FIFO.sem_signal();
    }
}

void funcion_hebra_consumidora_FIFO(int num_hebra) {
    int prox;
    for (unsigned i = num_hebra; i < num_items; i += num_consumidores) {
        int dato;
        puede_leer_FIFO.sem_wait();

        dato = buffer_FIFO[leer_FIFO];
        prox = (leer_FIFO + 1) % tam_vec;
        leer_FIFO.store(prox, memory_order::memory_order_acquire);

        puede_escribir_FIFO.sem_signal();
        consumir_dato(dato, num_hebra);
    }
}

//------------------------------------------------------------------------------

/**
 * @brief Ejecuta el programa dependiendo de la versión del problema
 * @param type: tipo de problema a ejecutar
 */
int ejecutar(int tipo) {
    if (tipo != 0 && tipo != 1) {
        cout << "Error: no ha indicado un número válido";
        return 1;
    }

    thread hebra_consumidora[num_consumidores];
    thread hebra_productora[num_productores];

    if (tipo == 0) {
        cout << "Ejecutando solución LIFO..." << endl;
        for (int i = 0; i < num_consumidores; i++)
            hebra_consumidora[i] = thread(funcion_hebra_consumidora_LIFO, i);
        for (int i = 0; i < num_productores; i++)
            hebra_productora[i] = thread(funcion_hebra_productora_LIFO, i);
    } else {
        cout << "Ejecutando solución FIFO..." << endl;
        for (int i = 0; i < num_consumidores; i++)
            hebra_consumidora[i] = thread(funcion_hebra_consumidora_FIFO, i);
        for (int i = 0; i < num_productores; i++)
            hebra_productora[i] = thread(funcion_hebra_productora_FIFO, i);
    }

    for (int i = 0; i < num_consumidores; i++) hebra_consumidora[i].join();
    for (int i = 0; i < num_productores; i++) hebra_productora[i].join();

    test_contadores();

    return 0;
}

int main() {
    cout << "¿Qué versión del problema desea ejecutar?" << endl
         << "   0 : versión LIFO" << endl
         << "   1 : versión FIFO" << endl
         << "Inserte la versión aquí -> ";
    int program_type;
    cin >> program_type;

    return ejecutar(program_type);
}
