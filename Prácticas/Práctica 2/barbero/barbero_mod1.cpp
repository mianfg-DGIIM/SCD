/**
 * @brief Problema del barbero
 * 
 * Versión para varios barberos y varios clientes, con número de asientos en
 * sala de espera y para un máximo de pelados
 * 
 * @author Miguel Ángel Fernández Gutiérrez <mianfg@correo.ugr.es>
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

mutex write_mtx;                    // cerrojo de stdout
constexpr int num_clientes = 6,     // número de clientes
    tipos_clientes = 2,             // número de tipos de clientes
    num_barberos = 1,               // número de barberos
    num_asientos = 3;               // número de asientos en sala de espera para cada tipo de cliente
        // suponemos que hay el mismo número de asientos que de clientes de cada tipo

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
 * @brief Monitor de barbería
 * 
 * @author Miguel Ángel Fernández Gutiérrez
 */
class Barberia : public HoareMonitor {
private:
    /**
     * @brief Indica el barbero que pelará al siguiente cliente
     */
    int siguiente_barbero;

    /**
     * @brief Siguiente tipo de cliente que pelará el barbero
     */
    int siguiente_tipo_cliente;

    /**
     * @brief Cola en la que duerme el barbero
     */
    CondVar barbero;

    /**
     * @brief Colas en la que esperan las hebras cliente si la silla está
     * ocupada
     */
    CondVar salaEspera[tipos_clientes];

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
    void cortarPelo(int num_cliente, int tipo_cliente);

    /**
     * @brief Acción de llamar al siguiente cliente
     * @param num_barbero: barbero que llama al siguiente cliente
     */
    void siguienteCliente(int num_barbero);

    /**
     * @brief Acción de acabar de pelar a un cliente
     * @param num_barbero: barbero que termina de pelar al cliente
     */
    void finCliente(int num_barbero);
};

Barberia::Barberia() {
    siguiente_barbero = -1;
    siguiente_tipo_cliente = 0;
    barbero = newCondVar();
    for ( unsigned i = 0; i < tipos_clientes; i++ )
        salaEspera[i] = newCondVar();
    for ( unsigned i = 0; i < num_barberos; i++ )
        silla[i] = newCondVar();
}

void Barberia::cortarPelo(int num_cliente, int tipo_cliente) {
    write_mtx.lock();
    cout << "\tEl cliente " << num_cliente << " (tipo " << tipo_cliente << ")"
         << " entra a que le corten el pelo" << endl;
    write_mtx.unlock();

    // si hay algún barbero dormido, lo despierta
    if ( !barbero.empty() && tipo_cliente == siguiente_tipo_cliente ) {
        write_mtx.lock();
        cout << "\tEl cliente " << num_cliente << " (tipo " << tipo_cliente << ")"
             << " despierta al barbero" << endl;
        write_mtx.unlock();
        barbero.signal();
    }

    // si la barbería está llena, sale
    // no limitaremos el número de asientos
    /*else if ( salaEspera[tipo_cliente].get_nwt() == num_asientos ) {
        
        write_mtx.lock();
        cout << "\tEl cliente " << num_cliente << " sale de la barbería, "
             << "porque está llena" << endl;
        write_mtx.unlock();

        return;
    }*/

    // en otro caso, quedará esperando su turno
    else {
        write_mtx.lock();
        cout << "\tEl cliente " << num_cliente << " (tipo " << tipo_cliente
             << ") acaba de entrar en la sala de espera." << endl;
        write_mtx.unlock();

        salaEspera[tipo_cliente].wait();
    }

    write_mtx.lock();
    cout << "\tEl cliente " << num_cliente << " (tipo " << tipo_cliente << ") entra a pelarse con el barbero "
         << siguiente_barbero << endl;
    write_mtx.unlock();

    silla[siguiente_barbero].wait();

    siguiente_tipo_cliente = (siguiente_tipo_cliente+1)%tipos_clientes;
}

void Barberia::siguienteCliente(int num_barbero) {
    // elige al cliente del tipo siguiente
    // si no hay clientes esperando
    if ( salaEspera[siguiente_tipo_cliente].get_nwt() == 0 ) {
        write_mtx.lock();
        cout << "El barbero " << num_barbero << " duerme, porque no hay clientes"
             << " de tipo " << siguiente_tipo_cliente << endl;
        write_mtx.unlock();
        barbero.wait();
        
        write_mtx.lock();
        cout << "El barbero " << num_barbero << " se despierta" << endl;
        write_mtx.unlock();

        siguiente_barbero = num_barbero;    // ahora el que pela es este barbero
    } else {
        siguiente_barbero = num_barbero;
        salaEspera[siguiente_tipo_cliente].signal();   // llama a un cliente
    }
}

void Barberia::finCliente(int num_barbero) {
    write_mtx.lock();
    cout << "El barbero " << num_barbero << " ha terminado de pelar al cliente" << endl;
    write_mtx.unlock();

    silla[num_barbero].signal();
}

void hebra_barberos(MRef<Barberia> monitor, int num_barbero) {
    while (true) {
        // espera a un cliente
        monitor->siguienteCliente(num_barbero);

        // le corta el pelo al cliente (retraso)
        cortarPelo(num_barbero);

        // avisa al cliente de que le ha cortado el pelo
        monitor->finCliente(num_barbero);
    }
}

void hebra_clientes(MRef<Barberia> monitor, int num_cliente, int tipo_cliente) {
    while (true) {
        // cliente solicita que le corten el pelo
        monitor->cortarPelo(num_cliente, tipo_cliente);

        // al cliente le crece el pelo (retraso)
        crecerPelo(num_cliente);
    }
}


int main() {
    cout << "___________________________________________________" << endl
         << endl << "PROBLEMA DEL BARBERO | MONITOR SU" << endl
         << "Alumno: Miguel Ángel Fernández Gutiérrez" << endl
         << "___________________________________________________" << endl << endl;

    // aclarar de qué tipo es cada cliente
    int tipo = 0;
    for ( int i = 0; i < num_clientes; i++ ) {
        cout << "Cliente " << i << " es de tipo " << tipo << endl;
        tipo = (tipo+1)%tipos_clientes;
    }
    cout << endl;

    MRef<Barberia> monitor = Create<Barberia>();

    thread hebra_b[num_barberos], hebra_c[num_clientes];
    for ( int i = 0; i < num_barberos; i++ )
        hebra_b[i] = thread(hebra_barberos, monitor, i);

    tipo = 0;
    for ( int i = 0; i < num_clientes; i++ ) {
        hebra_c[i] = thread(hebra_clientes, monitor, i, 1);
        tipo = (tipo+1)%tipos_clientes;
    }
    
    for ( int i = 0; i < num_barberos; i++ ) hebra_b[i].join();
    for ( int i = 0; i < num_clientes; i++ ) hebra_c[i].join();

    return 0;
}