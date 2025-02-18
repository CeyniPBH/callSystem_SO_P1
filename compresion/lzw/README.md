# LZW Compression Tool
Este programa es una implementacion en C++ del algoritmo de compresión Lempel-Ziv-Welch (LZW), utilizado para la compresión de archivos. Con un enfoque basado en diccionarios dinámicos para codificar secuencias repetitivas de datos.

El algoritmo de compresión sin perdida LZW funciona mediante la construcción de un diccionario dinámico mientras procesa el archivo, y asigna códigos numéricos a secuencias de caracteres repetidos. Estos códigos se escriben en el archivo comprimido.

### Compresión:
El diccionario se inicializa con los caracteres ASCII del 0 al 255, teniendo cada carácter un código único.

El archivo se lee carácter por carácter, ampliando el buffer con el siguiente carácter. Si la secuencia actual existe en el diccionario, se continúa acumulando caracteres. 

Si no existe, se agrega el código correspondiente de la secuencia anterior al archivo comprimido y se agrega la nueva secuencia al diccionario. Cuando se termina de leer el archivo, se agrega cualquier secuencia restante al archivo comprimido.

### Decompresion:
El archivo comprimido se lee y se extraen los códigos numéricos que representan las secuencias.  Usando el diccionario inicial, se reconstruye la secuencia de caracteres, añadiendo cada secuencia al archivo de salida.

A medida que se reconstruyen las secuencias, se agrega al diccionario nuevas combinaciones de secuencias.

## Opciones

-   **`-h` o `--help`**: Muestra el mensaje de ayuda.
-   **`-v` o `--version`**: Muestra la versión actual del programa.
-   **`-c <archivo>` o `--compress <archivo>`**: Comprime el archivo especificado y genera un archivo con la extensión `.lzw`.
-   **`-x <archivo>` o `--decompress <archivo>`**: Descomprime el archivo especificado, siempre que tenga la extensión `.lzw`.


## Uso
### Compresion de un archivo:

    lzw -c archivo.txt
### Descompresión de un archivo:

    lzw -x archivo.txt.lzw
### Ejemplo de ejecucion:

    $ lzw -c ejemplo.txt
    Archivo comprimido exitosamente como: ejemplo.txt.lzw

    $ lzw -x ejemplo.txt.lzw
    Archivo descomprimido exitosamente como: ejemplo.txt


## FAQ
### ¿Por que se usan archivos de texto?
El algoritmo de compresion LZW funciona especialmente bien en archivos de texto debido a la frecuencia en que se suelen encontrar secuencias repetidas.  Los archivos *txt* son ampliamente soportados y no requieren ningún formato especial para ser leídos o escritos, lo que los hace fácilmente intercambiables entre plataformas y aplicaciones.

### **¿Qué cambios harías si quisieras comprimir archivos binarios en lugar de texto?**

Si se tratara de archivos binarios, se necesitarían algunos ajustes:

Usar un diccionario basado en bytes en lugar de caracteres
En lugar de procesar cadenas, se pueden usar directamente secuencias de bytes (`std::vector<uint8_t>` en C++).

La lectura debe hacerse en modo binario, asegurando que no haya conversión de caracteres.


## Consideraciones
- Si el archivo de entrada ya esta altamente comprimido, el tamaño resultante puede ser superior al del archivo original.
- Esto también aplica en archivos que carecen de estructura o patrones frecuentes.
- Se puede aumentar el tamaño del diccionario y modificar la lógica del código para capturar secuencias mas largas como palabras, esto es especialmente útil en archivos que emplean muchas etiquetas, como HTML o PDF.

