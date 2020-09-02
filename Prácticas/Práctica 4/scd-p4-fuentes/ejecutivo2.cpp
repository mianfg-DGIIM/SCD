// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Práctica 4. Implementación de sistemas de tiempo real.
//
// Archivo: ejecutivo2.cpp
//
//   Datos de las tareas:
//   ------------
//   Ta.  T    C
//   ------------
//   A   500  100
//   B   500  150
//   C  1000  200
//   D  2000  240
//  -------------
//
//  Planificación (con Ts == 500 ms)
//  *---------*----------*---------*--------*
//  | A B C   | A B D    | A B C   | A B    |
//  *---------*----------*---------*--------*
//
//
// Historial:
// Creado en Diciembre de 2017
// -----------------------------------------------------------------------------

/**
 * Alumno: Miguel Ángel Fernández Gutiérrez
 * 
 * Sistemas Concurrentes y Distribuidos
 * Doble Grado en Ingeniería Informática y Matemáticas 2019/20, Grupo 2
 * _____________________________________________________________________________
 *
 * 
 * RESPUESTAS A LAS PREGUNTAS
 * --------------------------
 * 
 * ¿Cuál es el mínimo tiempo de espera que queda al final de las iteraciones del
 * ciclo secundario con tu solución?
 *  --> El ciclo secundario es de 500 ms, y según la planificación especificada,
 *      el mínimo tiempo de espera tras el final de las iteraciones del ciclo
 *      secundario será de 10 ms.
 * 
 *      Esto es así debido al segundo ciclo, donde tenemos las tareas A, B y D,
 *      que suman en total 490 ms. Esta diferencia es la mínima.
 *
 * ¿Sería planificable si la tarea D tuviese un tiempo cómputo de 250 ms?
 *  --> Sería posible, de acuerdo a nuestra planificación. En ese caso, el
 *      tiempo de espera tras las iteraciones sería de 0 ms (en el segundo
 *      ciclo).
 */

#include <chrono>    // utilidades de tiempo
#include <iostream>  // cout, cerr
#include <ratio>     // std::ratio_divide
#include <string>
#include <thread>

using namespace std;
using namespace std::chrono;
using namespace std::this_thread;

// tipo para duraciones en segundos y milisegundos, en coma flotante:
typedef duration<float, ratio<1, 1>> seconds_f;
typedef duration<float, ratio<1, 1000>> milliseconds_f;

// -----------------------------------------------------------------------------
// tarea genérica: duerme durante un intervalo de tiempo (de determinada
// duración)

void Tarea(const std::string &nombre, milliseconds tcomputo) {
    cout << "   Comienza tarea " << nombre << " (C == " << tcomputo.count()
         << " ms.) ... ";
    sleep_for(tcomputo);
    cout << "fin." << endl;
}

// -----------------------------------------------------------------------------
// tareas concretas del problema:

void TareaA() { Tarea("A", milliseconds(100)); }
void TareaB() { Tarea("B", milliseconds(150)); }
void TareaC() { Tarea("C", milliseconds(200)); }
void TareaD() { Tarea("D", milliseconds(240)); }

// -----------------------------------------------------------------------------
// implementación del ejecutivo cíclico:

int main(int argc, char *argv[]) {
    // Ts = duración del ciclo secundario
    const milliseconds Ts(500);

    // ini_sec = instante de inicio de la iteración actual del ciclo secundario
    time_point<steady_clock> ini_sec = steady_clock::now();

    while (true) {  // ciclo principal
        cout << endl
             << "---------------------------------------" << endl
             << "Comienza iteración del ciclo principal." << endl;

        for (int i = 1; i <= 4; i++) {  // ciclo secundario (4 iteraciones)
            cout << endl
                 << "Comienza iteración " << i << " del ciclo secundario."
                 << endl;

            switch (i) {
                case 1:
                    TareaA();
                    TareaB();
                    TareaC();
                    break;
                case 2:
                    TareaA();
                    TareaB();
                    TareaD();
                    break;
                case 3:
                    TareaA();
                    TareaB();
                    TareaC();
                    break;
                case 4:
                    TareaA();
                    TareaB();
                    break;
            }

            // calcular el siguiente instante de inicio del ciclo secundario
            ini_sec += Ts;

            // esperar hasta el inicio de la siguiente iteración del ciclo
            // secundario
            sleep_until(ini_sec);

            // (como antes) comprobar retraso y abortar si es mayor a 20 ms
            time_point<steady_clock> comprobar = steady_clock::now();
            time_point<steady_clock>::duration retraso = comprobar - ini_sec;
            cout << "Retraso respecto instante final esperado: "
                 << milliseconds_f(retraso).count() << " ms" << endl;

            if (retraso > milliseconds(20)) {
                cout << "ERROR: retraso superior a 20 ms, abortando..." << endl;
                exit(EXIT_FAILURE);
            }
        }
    }

    exit(EXIT_SUCCESS);
}
