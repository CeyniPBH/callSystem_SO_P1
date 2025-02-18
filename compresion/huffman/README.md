# Compresión de Archivos con Huffman C++
Este programa trabaja la compresión y descompresión de archivos utilizando de base el algoritmo de Huffman. Se ejecuta desde la línea de comandos y llama al sistema para trabajar los archivos en Linux.

## Estructura del programa.
1. **Definición de la estructura del árbol de Huffman.**
Genera los nodos, cada nodo tiene un caracter, su frecuencia de aparición en el texto y punteros a sus nodos hijos izquierdo y derecho. Lo cual es esencial para la construcción y codificación del archivo`.txt`.

2. **Construcción del árbol.**
Al ya definir la estructura, procedemos a construir el árbol utilizando las colas de prioridad que ordena los nodos según la frecuencia de los caracteres, combinando nodos de menor a mayor rango.

3. **Generador de códigos.**
Una vez creado el árbol, generamos un mapa desordenado llamado *codigo* que guarda los valores al recorrer el arbol, dando notación de `1`a la derecha y `0` a la izquierda.

4. **Comprimir archivo.**
El programa entra al archivo, lee y cuenta las frecuencuas de los caracteres, construye y genera el código. Luego con el código generado comprime el archivo pasando los datos de bytes a bits y guarda los datos comprimidos en un nuevo archivo `.huff`. Adicional guardando metadatos, ocmo el númeor de caracteres distintos o sobrantes que serviran para permitir la descompresión del archivo.

## Requisitos.

- **Sistema operativo:** Linux
- **Compilador:** GCC o similar
- **Librerías:** Librerías estandar (<iostream>, <fstream>, <queue>, <unordered_map>, <vector>, <string>).

## Uso

Ejecuta el programa con los siguientes comandos.

### Opciones:
```bash
Opciones:
  -h, --help: Mostrar este mensaje de ayuda.
  -v, --version: Mostrar información sobre el autor del programa.
  -c. --compress: Comprimri el archivo.
  -x, --decompress: Descomprimir el archivo.
```
### Ejemplos:
- Para mostrar ayuda.
```bash
  ./huffman -h ó
  ./huffman --help
```
- Para mostrar la versión.
```bash
  ./huffman -v ó
  ./huffman --version
```
- Para comprimir archivo.
```bash
  ./huffman -c archivo.txt ó
  ./huffman --compress archivo.txt
```
- Para descomprimir archivo.
```bash
  ./huffman -x archivo.huff ó
  ./huffman --decompress archivo.huff
```
### Output:
- Para mostrar ayuda.
```bash
  Opciones:
  -h, --help: Mostrar este mensaje de ayuda.
  -v, --version: Mostrar información sobre el autor del programa.
  -c. --compress: Comprimri el archivo.
  -x, --decompress: Descomprimir el archivo.
```
- Para mostrar la versión.
```bash
  "Compresor 1.0"
```
- Para mostrar el archivo comprimido.
```bash
  "Archivo comprimido con éxito como: archivo.huff"
```
- Para mostrar el archivo descomprimido.
```bash
  "Archivo descomprimido con éxito como: archivo"
``` 
## Detalles Técnicos.
- **Llamadas al sistema.**
`open()`, `read()`, `ẁrite()`, `close()` para la manipulación del archivo.
- **Padding.**
Sirve para agregar un relleno al final del archivo comprimido, que agrupa correctamente los bits en bytes.

## Notas Importantes.
- **Formatos de archivo comprimido:** Los archivos comprimidos tienen una extresión `.huff`. Contiene tanto los metadatos (frecuencias y mapas) como los datos comprimidos del archivo original.
- **Solo sirve para formatos (`.txt`):** Como dice, solo comprime y descomprime formatos `.txt`.
- **Compatibilidad:** Este programa solo se ha diseñado para funcionar en sistemas Linux, no sabemos si funciona en otro sistema operativo.
- **Uso de memoria dinámica:** Para la creación del arbol Huffman, se utiliza una memoria dinámica que debe ser liberada al finalizar el programa.
