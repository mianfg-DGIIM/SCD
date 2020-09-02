`SCD` **Sistemas Concurrentes y Distribuidos**

# Examen práctica 1

## Ejercicio 1: fumadores

Partiendo del algoritmo desarrollado durante la Práctica 1 para resolver el problema de los Fumadores usando semáforos, se pide modificar el algoritmo para satisfacer los siguientes requisitos.

Deseamos añadir una nueva hebra a nuestro problema llamada proveedor, que genera un ingrediente en bucle. Una vez ha generado uno, el proveedor despierta al estanquero y le pasa el ingrediente, el cual lo colocará en el mostrador y despertará al fumador correspondiente.

La hebra proveedora no generará nuevos ingredientes hasta que se haya retirado del mostrador el ingrediente ya producido. Las hebras de los fumadores no cambian con respecto al problema original. Recuerda controlar la sincronización entre todas las hebras mediante el uso de semáforos para evitar incoherencias en la salida o interbloqueos (_deadlocks_). 

## Ejercicio 2: fumadores

Modificar el problema de los fumadores de la siguiente manera:

* Se debe crear una nueva hebra estanquero, que se alternará con la otra hebra estanquero para el ciclo normal de producir ingrediente y esperar a ser recogido por los fumadores. En la hebra `main` se decidirá de forma aleatoria cuál es la hebra estanquero que comienza poniendo el primer ingrediente.

* Cada vez que un estanquero genere el mismo ingrediente que generó en su ciclo anterior, se deberá mostrar el siguiente mensaje por pantalla indicándolo:

  > El estanquero N ha producido dos veces seguidas el mismo ingrediente INGR

  Sustituyendo N por el número del estanquero e INGR por el ingrediente generado.

## Ejercicio 3: 