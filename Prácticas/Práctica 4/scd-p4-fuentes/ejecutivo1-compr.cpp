// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Práctica 4. Implementación de sistemas de tiempo real.
//
// Archivo: ejecutivo1-compr.cpp
// Implementación del primer ejemplo de ejecutivo cíclico:
//
//   Datos de las tareas:
//   ------------
//   Ta.  T    C
//   ------------
//   A  250  100
//   B  250   80
//   C  500   50
//   D  500   40
//   E 1000   20
//  -------------
//
//  Planificación (con Ts == 250 ms)
//  *---------*----------*---------*--------*
//  | A B C   | A B D E  | A B C   | A B D  |
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
 * Cada vez que acaba un ciclo secundario, se informa del retraso del instante
 * final actual respecto al instante final esperado. Si se comprueba que dicho
 * retraso es superior a 20 milisegundos, aborta con un mensaje de error
 *  --> Ver NOTA 1 (línea 138)
 *
 * Verifica que el programa realmente aborta cuando debe, aumentando el tiempo
 * de cómputo de alguna tarea a un valor superior al establecido en la tabla, de
 * forma que el fin del ciclo secundario se retrase más de 20 ms
 *  --> Ver NOTA 2 (línea 80)
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

// NOTA 2: basta modificar el tiempo de cómputo de la tarea B a 200 ms, i.e.
void TareaA() { Tarea("A", milliseconds(100)); }
void TareaB() { Tarea("B", milliseconds(80)); }
void TareaC() { Tarea("C", milliseconds(50)); }
void TareaD() { Tarea("D", milliseconds(40)); }
void TareaE() { Tarea("E", milliseconds(20)); }

// -----------------------------------------------------------------------------
// implementación del ejecutivo cíclico:

int main(int argc, char *argv[]) {
    // Ts = duración del ciclo secundario
    const milliseconds Ts(250);

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
                    TareaE();
                    break;
                case 3:
                    TareaA();
                    TareaB();
                    TareaC();
                    break;
                case 4:
                    TareaA();
                    TareaB();
                    TareaD();
                    break;
            }

            // calcular el siguiente instante de inicio del ciclo secundario
            ini_sec += Ts;

            // esperar hasta el inicio de la siguiente iteración del ciclo
            // secundario
            sleep_until(ini_sec);

            // NOTA 1: comprobar retraso y abortar si es mayor a 20 ms
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
