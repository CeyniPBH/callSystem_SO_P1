# Compresión y Descompresión de archivos con algoritmos.

Se trabajaron 2 tipos de algorimtmos para compresión y descompresión de archivos, los cuales son:

## El algoritmo de Huffman.
Este [algoritmo](compresion/huffman) trabaja con:
  1. *Frecuencia de caracteres:* hace una lectura por todo el archivo, mientras cuenta las veces que se repiten los caracteres. 
  2. *Arbol de Huffman:* Se encarga de construir un arbol de prioridades binario, que consiste en dejar los caracteres más frecuentes cerca de la raíz y los menos frecuentes más lejanos.
  3. *Construcción de código:* Aquí se recorre el árbol para asignar códigos a cada símbolo.

Ventajas:  
  1. *Compresión sin pérdida:* los datos originales se mantienen en base de los datos compartidos.
  2. *Eficiencia:* los caracteres más frecuentes tienen código más corto, lo que redice el tamaño del archivo de datos comprimidos.
  3. *Simplicidad:* Es trabajable y fácil de entender su estructura principal.

## El algoritmo de LZW.
