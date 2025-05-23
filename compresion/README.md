# Compresión y Descompresión de archivos con algoritmos.

Se trabajaron 2 tipos de algoritmos para compresión y descompresión de archivos, los cuales son:

## El algoritmo de Huffman.
Este [algoritmo](https://github.com/CeyniPBH/callSystem_SO_P1/tree/main/compresion/huffman) trabaja con:
  1. *Frecuencia de caracteres:* hace una lectura por todo el archivo, mientras cuenta las veces que se repiten los caracteres. 
  2. *Arbol de Huffman:* Se encarga de construir un arbol de prioridades binario, que consiste en dejar los caracteres más frecuentes cerca de la raíz y los menos frecuentes más lejos.
  3. *Construcción de código:* Aquí se recorre el árbol para asignar códigos binarios a cada caracter.

Ventajas:  
  1. *Compresión sin pérdida:* los datos originales se mantienen en base de los datos compartidos.
  2. *Eficiencia:* los caracteres más frecuentes tienen código más corto, lo que reduce el tamaño del archivo de datos comprimidos.
  3. *Simplicidad:* Es trabajable y fácil de entender su estructura principal.

## El algoritmo de LZW.
Este segundo [algoritmo](https://github.com/CeyniPBH/callSystem_SO_P1/tree/main/compresion/LZW%20Algorithm) 

Ventajas:
  1. Eficiencia con patrones persistentes como los archivos `CSV, HTML, PDF`.

Desventajas:
  1. No funciona muy bien con archivos cuyos caracteres se salen del límite del diccionario o poca organización de la estructura.
