# Journal
Este es un registro de los cambios que se han realizado a lo largo del proyecto.

## Dia 1
Se toma inspiración en el proyecto propuesto en https://levelup.gitconnected.com/malloc-is-not-magic-implementing-my-own-memory-allocator-e0354e914402

Se basa la arquitectura de la memoria en bloques en el articulo mencionado, y se implementa un primer prototipo de malloc y free.

## Dia 2
Se desaea implementar un sistema de test para el proyecto utilizando la libreria GTest. Por lo cual se crea un header para el proyecto el cual exporta las funciones de malloc y free.
Además se crea el archivo CMakeLists.txt para poder crear la librería.

## Dia 3
Se agrega la herramienta cppcheck para poder analizar el código y detectar errores de sintaxis y estilo. Además, se realiza un análisis del código utilizando Claude-Code.
Mi idea no es utilziar el agente para crear el código, sino que este se comporte como un pair para poder detectar errores y mejorar el código.
Esto revela varios problemas de lógica:
 - Hacer free de un bloque en la mitad rompia la estructura de la memoria
 - Se detecta que el código no es thread-safe
 - Se resalta la falta de manejo de errores en la función sbrk, lo cual puede causar problemas si el sistema no puede asignar memoria.
 - Se detecta que al momento de asignar un bloque y buscar dividirlo para crear otro, se podría romper la estructura de la memoria al tratar de expandir el heap.

## Dia 4
Se agregan las librerías pthread y stdlib para poder utilizar mutex y la función exit, respectivamente. Esto permite que el código sea thread-safe y que se pueda manejar errores de manera más adecuada.
Además, se implementa una función manage_heap que envuelve la llamada a sbrk y maneja los errores de manera adecuada, saliendo del programa si no se puede asignar memoria.

