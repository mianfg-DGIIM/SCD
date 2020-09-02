/**
 * @brief Problema del productor-consumidor
 * Multihebra, monitor semántica SU, versiones FIFO y LIFO
 * @author Miguel Ángel Fernández Gutiérrez <mianfg@correo.ugr.es>
 *
 * Sistemas Concurrentes y Distribuidos
 * 3º Doble Grado en Ingeniería Informática y Matemáticas, 2019/2020
 */
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

constexpr int num_items = 40;   // número de items a producir/consumir
unsigned cont_prod[num_items],  // contadores de verificación: producidos
    cont_cons[num_items];       // contadores de verificación: consumidos
const int num_productores = 3,  // número de productores
    num_consumidores = 3;       // número de consumidores
mutex mtx;                      // mutex para stdout

// almacena cuántos ítems ha producido/consumido cada hebra
unsigned items_producidos[num_productores] = {0},
    items_consumidos[num_consumidores] = {0};

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
    items_producidos[num_hebra]++;
    this_thread::sleep_for(chrono::milliseconds(aleatorio<20, 100>()));
    mtx.lock();
    cout << "producido: " << contador << " (hebra " << num_hebra << ", total: "
         << items_producidos[num_hebra] << ")" << endl << flush;
    mtx.unlock();
    cont_prod[contador]++;
    return contador++;
}

/**
 * @brief Consume un dato
 * @param dato: dato que consume
 * @param num_hebra: índice de la hebra
 */
void consumir_dato(unsigned dato, int num_hebra) {
    if (num_items <= dato) {
        cout << " dato == " << dato << ", num_items == " << num_items << endl;
        assert(dato < num_items);
    }
    cont_cons[dato]++;
    items_consumidos[num_hebra]++;
    this_thread::sleep_for(chrono::milliseconds(aleatorio<20, 100>()));
    mtx.lock();
    cout << "                  consumido: " << dato << " (hebra " << num_hebra
         << ", total: " << items_consumidos[num_hebra] << ")" << endl;
    mtx.unlock();
}

/**
 * @brief Inicia contadores de comprobación
 */
void ini_contadores() {
    for (unsigned i = 0; i < num_items; i++) {
        cont_prod[i] = 0;
        cont_cons[i] = 0;
    }
}

/**
 * @brief Comprobador de contadores
 */
void test_contadores() {
    bool ok = true;
    cout << "comprobando contadores ...." << flush;

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

// == SOLUCIÓN FIFO ============================================================

/**
 * @brief Clase para monitor búfer, versión FIFO, semántica SU y múltiples
 * productores y consumidores
 */
class ProdConsSU_FIFO : public HoareMonitor {
private:
    static const int num_celdas_total = 10; // número de entradas del búfer
    int buffer[num_celdas_total],   // búfer
        primera_libre,              // índice de próxima inserción
        primera_ocupada,            // índice de próxima extracción
        num_celdas_ocupadas;        // número de celdas ocupadas

    // colas de condición
    CondVar ocupadas,               // cola donde espera consumidor
        libres;                     // cola donde espera productor

public:
    ProdConsSU_FIFO();
    int leer();
    void escribir(int valor);
};

ProdConsSU_FIFO::ProdConsSU_FIFO() {
    primera_libre = 0;
    primera_ocupada = 0;
    num_celdas_ocupadas = 0;
    ocupadas = newCondVar();
    libres = newCondVar();
}

int ProdConsSU_FIFO::leer() {
    // esperar bloqueado hasta que 0 < num_celdas_ocupadas
    if ( num_celdas_ocupadas == 0 )
        ocupadas.wait();

    // hacer operación de lectura, actualizando estado del monitor
    assert(0 < num_celdas_ocupadas);
    const int valor = buffer[primera_ocupada];
    primera_ocupada = (primera_ocupada + 1) % num_celdas_total;
    num_celdas_ocupadas--;

    // señalar al productor de que hay un hueco libre, por si está esperando
    libres.signal();

    // devolver valor
    return valor;
}

void ProdConsSU_FIFO::escribir(int valor) {
    // esperar bloqueado hasta que num_celdas_ocupadas < num_celdas_total
    if ( num_celdas_ocupadas == num_celdas_total )
        libres.wait();

    // cout << "escribir: ocup == " << num_celdas_ocupadas << ", total == "
    //      << num_celdas_total << endl;
    assert(num_celdas_ocupadas < num_celdas_total);

    // hacer la operación de inserción, actualizando estado del monitor
    buffer[primera_libre] = valor;
    primera_libre = (primera_libre + 1) % num_celdas_total;
    num_celdas_ocupadas++;

    // señalar al consumidor que ya hay una celda ocupada (por si está
    // esperando)
    ocupadas.signal();
}

// -----------------------------------------------------------------------------

/**
 * @brief Función hebra productora, versión FIFO
 * @param monitor: monitor productor consumidor semántica SU, versión FIFO
 * @param num_hebra: número de hebra
 */
void funcion_hebra_productora_FIFO(MRef<ProdConsSU_FIFO> monitor, int num_hebra) {
    for ( unsigned i = num_hebra; i < num_items; i += num_productores ) {
        int valor = producir_dato(num_hebra);
        monitor->escribir(valor);
    }
}

/**
 * @brief Función hebra consumidora, versión FIFO
 * @param monitor: monitor productor consumidor semántica SU, versión FIFO
 * @param num_hebra: número de hebra
 */
void funcion_hebra_consumidora_FIFO(MRef<ProdConsSU_FIFO> monitor, int num_hebra) {
    for ( unsigned i = num_hebra; i < num_items; i += num_consumidores ) {
        int valor = monitor->leer();
        consumir_dato(valor, num_hebra);
    }
}

// =============================================================================


// == SOLUCIÓN LIFO ============================================================

/**
 * @brief Clase para monitor búfer, versión LIFO, semántica SU y múltiples
 * productores y consumidores
 */
class ProdConsSU_LIFO : public HoareMonitor {
private:
    static const int num_celdas_total = 10; // número de entradas del búfer
    int buffer[num_celdas_total],   // búfer
        primera_libre;              // índice de próxima inserción

    // colas de condición
    CondVar ocupadas,               // cola donde espera consumidor
        libres;                     // cola donde espera productor

public:
    ProdConsSU_LIFO();
    int leer();
    void escribir(int valor);
};

ProdConsSU_LIFO::ProdConsSU_LIFO() {
    primera_libre = 0;
    ocupadas = newCondVar();
    libres = newCondVar();
}

int ProdConsSU_LIFO::leer() {
    // esperar bloqueado hasta que 0 < num_celdas_ocupadas
    if ( primera_libre == 0 )
        ocupadas.wait();

    // hacer operación de lectura, actualizando estado del monitor
    assert(0 < primera_libre);
    primera_libre--;
    const int valor = buffer[primera_libre];

    // señalar al productor de que hay un hueco libre, por si está esperando
    libres.signal();

    // devolver valor
    return valor;
}

void ProdConsSU_LIFO::escribir(int valor) {
    // esperar bloqueado hasta que num_celdas_ocupadas < num_celdas_total
    if ( primera_libre == num_celdas_total )
        libres.wait();

    // cout << "escribir: ocup == " << num_celdas_ocupadas << ", total == "
    //      << num_celdas_total << endl;
    assert(primera_libre < num_celdas_total);

    // hacer la operación de inserción, actualizando estado del monitor
    buffer[primera_libre] = valor;
    primera_libre++;

    // señalar al consumidor que ya hay una celda ocupada (por si está
    // esperando)
    ocupadas.signal();
}

// -----------------------------------------------------------------------------

/**
 * @brief Función hebra productora, versión LIFO
 * @param monitor: monitor productor consumidor semántica SU, versión FIFO
 * @param num_hebra: número de hebra
 */
void funcion_hebra_productora_LIFO(MRef<ProdConsSU_LIFO> monitor, int num_hebra) {
    for ( unsigned i = num_hebra; i < num_items; i += num_productores ) {
        int valor = producir_dato(num_hebra);
        monitor->escribir(valor);
    }
}

/**
 * @brief Función hebra consumidora, versión LIFO
 * @param monitor: monitor productor consumidor semántica SU, versión FIFO
 * @param num_hebra: número de hebra
 */
void funcion_hebra_consumidora_LIFO(MRef<ProdConsSU_LIFO> monitor, int num_hebra) {
    for ( unsigned i = num_hebra; i < num_items; i += num_consumidores ) {
        int valor = monitor->leer();
        consumir_dato(valor, num_hebra);
    }
}

// =============================================================================


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
        MRef<ProdConsSU_LIFO> monitor = Create<ProdConsSU_LIFO>();
        cout << "Ejecutando solución LIFO..." << endl;
        for (int i = 0; i < num_consumidores; i++)
            hebra_consumidora[i] = thread(funcion_hebra_consumidora_LIFO, monitor, i);
        for (int i = 0; i < num_productores; i++)
            hebra_productora[i] = thread(funcion_hebra_productora_LIFO, monitor, i);
    } else {
        MRef<ProdConsSU_FIFO> monitor = Create<ProdConsSU_FIFO>();
        cout << "Ejecutando solución FIFO..." << endl;
        for (int i = 0; i < num_consumidores; i++)
            hebra_consumidora[i] = thread(funcion_hebra_consumidora_FIFO, monitor, i);
        for (int i = 0; i < num_productores; i++)
            hebra_productora[i] = thread(funcion_hebra_productora_FIFO, monitor, i);
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