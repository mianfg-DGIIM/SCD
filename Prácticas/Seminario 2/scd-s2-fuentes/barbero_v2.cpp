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
constexpr int num_clientes = 10;    // número de clientes
constexpr int num_barberos = 2;     // número de barberos
constexpr int num_asientos = 3;     // número de asientos en sala de espera
constexpr int max_pelados = 4;      // máximo de pelados para cada barbero

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
 * @brief Función de cortar pelo
 * @param num_barbero: barbero que corta el pelo
 * 
 * Realiza una espera de tiempo aleatorio
 */
void cortarPelo(int num_barbero) {
    chrono::milliseconds time(aleatorio<20,100>());

    write_mtx.lock();
    cout << "Barbero " << num_barbero << " cortando el pelo del cliente... ("
         << time.count() << "ms)" << endl;
    write_mtx.unlock();

    this_thread::sleep_for(time);

    write_mtx.lock();
    cout << "El pelo del cliente ha sido cortado por el barbero "
         << num_barbero << endl;
    write_mtx.unlock();
}

/**
 * @brief Función de crecer pelo (estar fuera de la barbería)
 * @param num_cliente: número de la hebra cliente
 * 
 * Realiza una espera de tiempo aleatorio
 */
void crecerPelo(int num_cliente) {
    chrono::milliseconds time(aleatorio<20,100>());

    write_mtx.lock();
    cout << "\t\tA " << num_cliente << " le está creciendo el pelo ("
         << time.count() << "ms)" << endl;
    write_mtx.unlock();

    this_thread::sleep_for(time);
    
    write_mtx.lock();
    cout << "\t\tA " << num_cliente << " ya le ha crecido el pelo" << endl;
    write_mtx.unlock();
}

/**
 * @brief Función de descansar (permanecer un tiempo el barbero durmiendo al
 * llegar a un cierto número de pelados)
 * @param num_barbero: número de la hebra barbero
 */
void descansarBarbero(int num_barbero) {
    chrono::milliseconds time(aleatorio<20,100>());

    write_mtx.lock();
    cout << "El barbero " << num_barbero << " se merece un descanso ("
         << time.count() << "ms)" << endl;
    write_mtx.unlock();
    
    this_thread::sleep_for(time);

    write_mtx.lock();
    cout << "¡El barbero " << num_barbero << " está listo para volver a "
         << "trabajar!" << endl;
    write_mtx.unlock();
}

/**
 * @brief Monitor de barbería
 * 
 * @author Miguel Ángel Fernández Gutiérrez
 */
class Barberia : public HoareMonitor {
private:
    /**
     * @brief Contador de clientes que ha pelado cada barbero
     */
    unsigned pelados[num_barberos];

    /**
     * @brief Indica el barbero que pelará al siguiente cliente
     */
    int siguiente_barbero;

    /**
     * @brief Cola en la que duerme el barbero
     */
    CondVar barbero;

    /**
     * @brief Cola en la que esperan las hebras cliente si la silla está
     * ocupada
     */
    CondVar salaEspera;

    /**
     * @brief Cola que representa si hay alguien pelándose
     */
    CondVar silla[num_barberos];
public:
    /**
     * @brief Constructor por defecto
     */
    Barberia();

    /**
     * @brief Acción de cortar el pelo
     * @param num_cliente: cliente al que se le corta el pelo
     */
    void cortarPelo(int num_cliente);

    /**
     * @brief Acción de llamar al siguiente cliente
     * @param num_barbero: barbero que llama al siguiente cliente
     */
    void siguienteCliente(int num_barbero);

    /**
     * @brief Acción de acabar de pelar a un cliente
     * @param num_barbero: barbero que termina de pelar al cliente
     * 
     * @return si el barbero seguirá pelando o no (dependiendo de si ya ha
     * pelado el máximo)
     */
    bool finCliente(int num_barbero);
};

Barberia::Barberia() {
    siguiente_barbero = -1;
    barbero = newCondVar();
    salaEspera = newCondVar();
    for ( unsigned i = 0; i < num_barberos; i++ ) {
        silla[i] = newCondVar();
        pelados[i] = 0;
    }
}

void Barberia::cortarPelo(int num_cliente) {
    write_mtx.lock();
    cout << "\tEl cliente " << num_cliente << " entra a que le corten el pelo" << endl;
    write_mtx.unlock();

    // si hay algún barbero dormido, lo despierta
    if ( !barbero.empty() ) {
        write_mtx.lock();
        cout << "\tEl cliente " << num_cliente << " despierta al barbero" << endl;
        write_mtx.unlock();
        barbero.signal();
    }

    // si la barbería está llena, sale
    else if ( salaEspera.get_nwt() == num_asientos ) {
        /**WIP implementación alternativa
         * Hacer que espere en otra cola
         */
        write_mtx.lock();
        cout << "\tEl cliente " << num_cliente << " sale de la barbería, "
             << "porque está llena" << endl;
        write_mtx.unlock();

        return;
    }

    // en otro caso, quedará esperando su turno
    else {
        write_mtx.lock();
        cout << "\tEl cliente " << num_cliente << " acaba de entrar en la sala de espera." << endl;
        write_mtx.unlock();

        salaEspera.wait();
    }

    write_mtx.lock();
    cout << "\tEl cliente " << num_cliente << " entra a pelarse con el barbero "
         << siguiente_barbero << endl;
    write_mtx.unlock();

    silla[siguiente_barbero].wait();
}

void Barberia::siguienteCliente(int num_barbero) {
    // si no hay clientes esperando
    if ( salaEspera.get_nwt() == 0 ) {
        write_mtx.lock();
        cout << "El barbero " << num_barbero << " duerme, porque no hay clientes" << endl;
        write_mtx.unlock();
        barbero.wait();
        
        write_mtx.lock();
        cout << "El barbero " << num_barbero << " se despierta" << endl;
        write_mtx.unlock();

        siguiente_barbero = num_barbero;    // ahora el que pela es este barbero
    } else {
        siguiente_barbero = num_barbero;
        salaEspera.signal();                // llama a un cliente
    }
}

bool Barberia::finCliente(int num_barbero) {
    write_mtx.lock();
    cout << "El barbero " << num_barbero << " ha terminado de pelar al cliente" << endl;
    write_mtx.unlock();

    bool descansar = false;
    pelados[num_barbero]++;
    silla[num_barbero].signal();

    if ( pelados[num_barbero] == max_pelados ) {
        pelados[num_barbero] = 0;
        descansar = true;
    }

    return descansar;
}

void hebra_barberos(MRef<Barberia> monitor, int num_barbero) {
    bool descansar;
    while (true) {
        // espera a un cliente
        monitor->siguienteCliente(num_barbero);

        // le corta el pelo al cliente (retraso)
        cortarPelo(num_barbero);

        // avisa al cliente de que le ha cortado el pelo
        descansar = monitor->finCliente(num_barbero);

        // si el barbero ha de descansar
        if ( descansar )
            descansarBarbero(num_barbero);
    }
}

void hebra_clientes(MRef<Barberia> monitor, int num_cliente) {
    while (true) {
        // cliente solicita que le corten el pelo
        monitor->cortarPelo(num_cliente);

        // al cliente le crece el pelo (retraso)
        crecerPelo(num_cliente);
    }
}


int main() {
    cout << "___________________________________________________" << endl
         << endl << "PROBLEMA DEL BARBERO | MONITOR SU" << endl
         << "Alumno: Miguel Ángel Fernández Gutiérrez" << endl
         << "___________________________________________________" << endl << endl;

    MRef<Barberia> monitor = Create<Barberia>();

    thread hebra_b[num_barberos], hebra_c[num_clientes];
    for ( int i = 0; i < num_barberos; i++ )
        hebra_b[i] = thread(hebra_barberos, monitor, i);
    for ( int i = 0; i < num_clientes; i++ )
        hebra_c[i] = thread(hebra_clientes, monitor, i);
    
    for ( int i = 0; i < num_barberos; i++ ) hebra_b[i].join();
    for ( int i = 0; i < num_clientes; i++ ) hebra_c[i].join();

    return 0;
}