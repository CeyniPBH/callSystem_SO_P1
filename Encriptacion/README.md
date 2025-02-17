# Encriptador de Archivos en C++

Este programa en C++ permite encriptar y desencriptar archivos utilizando un algoritmo basado en la operación XOR. Se ejecuta desde la línea de comandos y usa llamadas al sistema para la manipulación de archivos en Linux.

## Algoritmo Utilizado: Cifrado XOR

El programa implementa un cifrado simple basado en la operación bit a bit XOR (`^`). La idea detrás del cifrado XOR es que aplicar la misma clave dos veces devuelve el mensaje original:

- **Encriptación**: `byte_encriptado = byte_original ^ XOR_KEY`
- **Desencriptación**: `byte_original = byte_encriptado ^ XOR_KEY`

Esto significa que el mismo código se puede utilizar tanto para encriptar como para desencriptar el archivo.

El programa utiliza un **tamaño de bloque de 1024 bytes** para procesar el archivo en partes y garantizar eficiencia en la manipulación de archivos grandes.

## Funcionalidad

1. **Copia el archivo original** a un archivo temporal (`archivo_encriptar_desencriptar.txt`).
2. **Aplica el cifrado XOR** a la copia y guarda el resultado en otro archivo temporal (`archivo_encriptado_desencriptado.txt`).
3. **Reemplaza el archivo original** con la versión encriptada o desencriptada.
4. **Elimina la copia temporal** del archivo original para mantener la limpieza del sistema.

## Requisitos

- Sistema operativo **Linux**
- Compilador **g++**
- Permisos de lectura y escritura en los archivos procesados

## Uso

Ejecuta el programa con las siguientes opciones:

### Mostrar ayuda
```bash
./Parcial1 -h
```

### Mostrar versión del programa
```bash
./Parcial1 -v
```

### Encriptar un archivo
```bash
./Parcial1 -e <archivo>
```
Ejemplo:
```bash
./Parcial1 -e archivo.txt
```
Después de ejecutar este comando, `archivo.txt` quedará encriptado.

### Desencriptar un archivo
```bash
./Parcial1 -d <archivo>
```
Ejemplo:
```bash
./Parcial1 -d archivo.txt
```
Después de ejecutar este comando, `archivo.txt` volverá a su estado original.

## Detalles Técnicos

- **Llamadas al sistema utilizadas:**
  - `open()`, `read()`, `write()`, `close()`: Para manipular archivos.
  - `system()`: Para ejecutar comandos del sistema (`cp`, `mv`, `rm`).
- **Permisos de archivos:**
  - `S_IRUSR | S_IWUSR` (`0600` en octal) para garantizar que solo el usuario pueda leer y escribir el archivo encriptado.

## Notas Importantes

- **Solo funciona para documentos de texto(.txt)** 
- Este cifrado **no es seguro para datos sensibles**, ya que XOR con clave fija es fácilmente reversible.
- **No hay recuperación de clave**: Si pierdes la clave usada en el código (`XOR_KEY`), no podrás recuperar los archivos encriptados.
- **Sobrescribe el archivo original**, por lo que es recomendable hacer una copia de seguridad antes de procesar archivos importantes.
