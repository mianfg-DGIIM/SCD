# Seminario 1

## Hebras en C++ 11

* Construidas sobre estándar de hebras POSIX, `lpthread`.

### Creación de hebras

~~~c++
#include <thread>

void func1() {...}
void func2(int a, char b) {...}

int main() {
    thread hebra1(func_1),	// en este momento estamos lanzando la hebra
           hebra2(func_2, 7, 'M');
	...
}
~~~

Si queremos lanzar la hebra en otro sitio:

~~~c++
thread hebra1, hebra2;
...
hebra1 = thread(func_1);
hebra2 = thread(func_2);
~~~

###### Compilación y enlace

~~~
g++ -std=c++11 -o ejecutable -lpthread fuente1.cpp fuente2.cpp ... fuenteN.cpp
~~~

> **Ejemplo 01**
>
> El programa acaba antes de que acaben las hebras, por tanto las hebras terminan de forma anormal. Debemos hacer que la hebra `main` espere a que terminen las hebras 1 y 2.



### Operación de unión

Hace falta hacer un `join` (**operación de unión**), que se invoca sobre otra hebra (no es como un `barrier`).

No se pueden hacer dos `join` sobre una hebra.

> **Ejemplo 02**
>
> Se soluciona el problema mediante los `join`. Los mensajes se entrelazan, e incluso se mezclan (la escritura en el búffer de salida no se da lugar en exclusión mutua).



### Obtención de los resultados de la ejecución de una hebra

* **Variables globales compartidas:** hebra A espera a que hebra B escriba en variable `v`, cuando A se asegura de que B ha terminado (_join_), A lee el contenido de `v`, obteniendo el resultado.

  * Problema: hebra B tiene efecto lateral; mientras B escribe en la variable puede que otra hebra lea o escriba, dando lugar a inconsistencias.

  > **Ejemplo 03**
  >
  > Se puede probar que dos hebras escriban el resultado en la misma variable. Como la segunda hebra tarda más que la primera, se escribe después (está calculando el factorial de un número mayor), por lo general.

* **Pasar un puntero o referencia como parámetro de función:** luego leemos el valor de dicha variable.

  * Problema: mismo que el anterior.

  > **Ejemplo 04**
  >
  > En este programa sólo se escribe una función, con la que podemos instanciar todas las hebras que queramos. Además, no usamos variables globales.

* **Futuros:** no creamos explícitamente la hebra B, sino que la función devuelve el valor con `return`, y lanzamos la función para que una hebra anónima la ejecute mediante `async` (que lanza una hebra que ejecuta la función y devuelve el objeto de tipo `future`) y guarde el resultado en un sitio seguro, accesible por un objeto de tipo `future`.

  * Más simple y legible, sin efectos laterales.

  > **Ejemplo 05**
  >
  > No hace falta usar `join`.



### Vectores de hebras y vectores de futuros

> **Ejemplo 06**
>
> Dentro del código de la hebra imprimimos el mensaje. Los resultados salen de forma desordenada, habría que intentar proteger los `cout` de cada hebra para que fuesen en exclusión mutua. De ese modo, un `cout` de una hebra no quedaría interrumpido por el `cout` de otra.

> **Ejemplo 07**
>
> El resultado es ordenado, porque esperamos a que las hebras terminen para imprimir el resultado.



### Medición de tiempos

Dos tipos de datos en `std::chrono`:

* `time_point`: tiempo transcurrido desde un instante de inicio.
* `duration`: diferencia entre dos instantes de tiempo.

Tres relojes:

* `system_clock`: tiempo indicado por hora/fecha del sistema. Puede cambiarse.
* `steady_clock`: toma un instante de inicio, y siempre avanza. Usaremos este.
* `high_precision_clock`: está implementado por uno de los dos anteriores.



> **Actividad.** Implementación multihebra de aproximación numérica de una integral





## Sincronización básica en C++11

Exclusión mutua necesaria cuando hebra accede a variable compartida, para bien leer o escribir, y otra hace lo mismo. Debemos asegurar el acceso **mutuamente excluyente** a esa dirección de memoria.

* **Tipos atómicos:** tipos de datos cuyas variables se actualizan de forma atómica.
  * Tipos genéricos que se instancian con tipos enteros: `atomic<T>`
  * Las operaciones `k=expr`, `k++`, `k--`, `k+=expr`, `k-=expr`. Si `expr` no es un literal simple, no se realiza de forma atómica.

> **Ejemplo 10**
>
> En el caso de `atomic` la espera es de grano más grueso (en ambos casos hay espera), las hebras se "estorban" más.

* **Objetos mutex o cerrojos:** tienen dos métodos, `lock` y `unlock`, para secciones críticas, evitando que se entrelacen. El orden de ejecución de cada sección crítica es indeterminado.
  * Si tenemos diferentes secciones críticas entre sí, creamos varios `mutex`; si usásemos el mismo `mutex` perderíamos concurrencia.



## Introducción a los semáforos

Estructura de datos que trabaja en memoria compartida. No existen los semáforos en sistemas distribuidos.

Inicialmente, al declarar un semáforo el **conjunto de procesos bloqueados** está vacío, debemos darle una inicialización.

| `sem_wait(s)`                                                | `sem_signal(s)`                                              |
| ------------------------------------------------------------ | ------------------------------------------------------------ |
| Un proceso la ejecuta cuando tiene motivos para esperarse. Dependiendo de la ejecución, esperará o no. También depende del valor del semáforo. | Desbloquea procesos o incrementa el valor del semáforo. Siempre se incrementa el semáforo. Si hay procesos bloqueados, se saca uno de ellos y se reanuda, se desbloquea. |

Si el semáforo tiene procesos bloqueados, está a 0 y al final `signal` sólo desbloquea, no incrementa. Incrementa sólo cuando no hay procesos bloqueados.

Ambas operaciones se ejecutan en exclusión mutua, excluyendo el período de bloqueo de `wait`. Si un proceso bloquea y no libera la exclusión mutua, se bloquearía todo el programa.

El valor de un semáforo se puede calcular:
$$
v_t=v_0+n_s-n_w\geq 0
$$


### Patrones de uso sencillos

* **Espera única:** un proceso debe esperar a otro para continuar.
* **Exclusión mutua:** secciones críticas.
  * Si `semaphore=2`, permitimos que dos hebras entren en la sección critica (_exclusión mutua relajada_).
* **Productor/consumidor:** espera única cruzada.



Los semáforos binarios son en el fondo cerrojos.