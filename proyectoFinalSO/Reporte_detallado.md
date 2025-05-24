# Informe Técnico: Sistema de Respaldo Paralelo con Dask

## 1. Introducción

Este documento presenta el análisis técnico de un sistema de respaldo y restauración desarrollado en Python que implementa paralelización mediante Dask, compresión de archivos y fragmentación para diferentes tipos de almacenamiento. El sistema está diseñado para manejar grandes volúmenes de datos de manera eficiente aprovechando el procesamiento paralelo.

## 2. Arquitectura del Sistema

### 2.1 Estructura General

El sistema está compuesto por dos módulos principales:

- **`backup_system.py`**: Contiene la lógica principal del sistema de respaldo
- **`cli.py`**: Interfaz de línea de comandos para interactuar con el sistema

### 2.2 Componentes Arquitectónicos

```
┌─────────────────────────────────────────────────────────────┐
│                    CLI Interface                            │
│                     (cli.py)                               │
└─────────────────┬───────────────────────────────────────────┘
                  │
┌─────────────────▼───────────────────────────────────────────┐
│                BackupSystem Class                          │
│               (backup_system.py)                           │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │
│  │   Dask      │  │ Compression │  │   File Operations   │  │
│  │ LocalCluster│  │   Engine    │  │    & Splitting      │  │
│  └─────────────┘  └─────────────┘  └─────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

### 2.3 Flujo de Operaciones

#### Proceso de Respaldo:
1. **Inicialización**: Configuración del cluster Dask
2. **Recolección**: Exploración paralela de directorios
3. **Compresión**: Creación del archivo comprimido
4. **Fragmentación**: División del archivo (opcional)
5. **Almacenamiento**: Guardado según el tipo de destino

#### Proceso de Restauración:
1. **Reconstrucción**: Unión de fragmentos (si aplica)
2. **Descompresión**: Extracción de archivos
3. **Restauración**: Colocación en directorio destino

## 3. Implementación del Paralelismo con Dask

### 3.1 Configuración del Cluster

El sistema utiliza Dask para implementar paralelización en múltiples niveles:

```python
self.cluster = LocalCluster(n_workers=os.cpu_count(), threads_per_worker=1)
self.client = Client(self.cluster)
```

**Características del cluster:**
- **Número de workers**: Igual al número de núcleos del CPU
- **Threads por worker**: 1 (para evitar conflictos de GIL)
- **Tipo**: LocalCluster para procesamiento en una sola máquina

### 3.2 Paralelización en Recolección de Archivos

La exploración de múltiples directorios se paraleliza usando decoradores `@delayed`:

```python
@delayed
def get_files_in_folder(folder_path: str) -> List[str]:
    filepaths = []
    for root, _, files in os.walk(folder_path):
        for file in files:
            filepaths.append(os.path.join(root, file))
    return filepaths

delayed_results = [get_files_in_folder(folder) for folder in source_folders]
all_files_nested = dask.compute(*delayed_results)
```

**Ventajas:**
- Cada directorio raíz se procesa en paralelo
- Reducción significativa del tiempo de exploración
- Escalabilidad automática según número de cores

### 3.3 Paralelización en Fragmentación

La escritura de fragmentos se paraleliza para mejorar el rendimiento de I/O:

```python
@delayed
def write_chunk(data_chunk: bytes, part_path: str):
    with open(part_path, 'wb') as f_part:
        f_part.write(data_chunk)
    return part_path

tasks = [write_chunk(chunk, part_path) for chunk, part_path in chunks_and_paths]
dask.compute(*tasks)
```

**Beneficios:**
- Escritura simultánea de múltiples fragmentos
- Mejor aprovechamiento del ancho de banda de I/O
- Reducción del tiempo total de fragmentación

### 3.4 Mecanismo de Fallback

El sistema incluye un mecanismo robusto de fallback:

```python
except Exception as e:
    logger.warning(f"No se pudo iniciar Dask LocalCluster, usando Dask en modo síncrono: {e}")
    dask.config.set(scheduler='synchronous')
```

Esto garantiza funcionamiento incluso en entornos con restricciones.

## 4. Algoritmos de Compresión

### 4.1 Algoritmos Implementados

El sistema soporta tres algoritmos de compresión clásicos:

#### ZIP (Deflate)
- **Biblioteca**: `zipfile` (estándar de Python)
- **Algoritmo**: Deflate (combinación de LZ77 y Huffman)
- **Ventajas**: Balance entre velocidad y compresión, amplia compatibilidad
- **Uso típico**: Archivos generales, documentos

#### GZIP (tar.gz)
- **Biblioteca**: `gzip` y `tarfile` (estándar de Python)
- **Algoritmo**: Deflate aplicado sobre archivo TAR
- **Ventajas**: Excelente para archivos de texto, estándar Unix/Linux
- **Proceso**: TAR → GZIP compression

#### BZIP2 (tar.bz2)
- **Biblioteca**: `bz2` y `tarfile` (estándar de Python)
- **Algoritmo**: Burrows-Wheeler Transform + Move-to-Front + Huffman
- **Ventajas**: Mayor ratio de compresión que GZIP
- **Desventajas**: Mayor uso de CPU y memoria

### 4.2 Implementación de Compresión

```python
def _compress_files(self, files_to_backup: List[str], archive_path: str, 
                   algorithm: str, source_folders: List[str]):
    if algorithm == 'zip':
        with zipfile.ZipFile(archive_path, 'w', zipfile.ZIP_DEFLATED) as zf:
            for file_path in files_to_backup:
                zf.write(file_path, arcname)
    elif algorithm in ['gzip', 'bzip2']:
        # Crear TAR intermedio, luego comprimir
        with tarfile.open(intermediate_tar_path, 'w') as tf:
            for file_path in files_to_backup:
                tf.add(file_path, arcname=arcname)
        # Comprimir el TAR
        with open(intermediate_tar_path, 'rb') as f_in, comp_module.open(archive_path, 'wb') as f_out:
            shutil.copyfileobj(f_in, f_out)
```

### 4.3 Preservación de Estructura de Directorios

El sistema mantiene la estructura relativa de directorios calculando rutas relativas:

```python
for base_dir in common_base_dirs:
    if file_path.startswith(base_dir):
        relative_to_parent_of_base = os.path.relpath(file_path, os.path.dirname(base_dir))
        arcname = relative_to_parent_of_base
        break
```

## 5. Justificación de Tecnologías

### 5.1 Selección del Lenguaje: Python

**Ventajas:**
- **Ecosistema rico**: Abundantes bibliotecas para I/O, compresión y paralelización
- **Dask nativo**: Python es el lenguaje principal para Dask
- **Facilidad de desarrollo**: Sintaxis clara y mantenimiento simplificado
- **Bibliotecas estándar**: `zipfile`, `tarfile`, `gzip`, `bz2` incluidas
- **Multiplataforma**: Funciona en Windows, Linux, macOS

**Consideraciones:**
- Python es interpretado, pero las operaciones intensivas (I/O, compresión) utilizan código C subyacente
- El GIL se mitiga usando procesos separados (Dask workers)

### 5.2 Selección de Dask sobre OpenMP

| Aspecto | Dask | OpenMP |
|---------|------|--------|
| **Integración** | Nativo en Python | Requiere extensiones C/C++ |
| **Facilidad de uso** | API Python intuitiva | Pragmas y gestión manual |
| **Escalabilidad** | Escalable a clusters | Limitado a memoria compartida |
| **Depuración** | Herramientas Python | Más complejo |
| **Overhead** | Moderado | Mínimo |
| **Flexibilidad** | Alta (task graphs) | Media (principalmente loops) |

**Decisión**: Dask por su integración natural con Python y flexibilidad para diferentes patrones de paralelización.

### 5.3 Bibliotecas Seleccionadas

#### Compresión
- **zipfile, gzip, bz2**: Bibliotecas estándar, no requieren instalación adicional
- **tarfile**: Manejo nativo de archivos TAR, esencial para gzip/bzip2

#### Paralelización
- **Dask**: Framework maduro con excelente documentación y comunidad activa
- **dask.distributed**: Client/Worker model escalable

#### Logging y CLI
- **logging**: Sistema robusto de logging integrado
- **argparse**: Parser profesional de argumentos de línea de comandos

## 6. Tipos de Almacenamiento Soportados

### 6.1 HDD (Disco Duro)
- **Uso**: Almacenamiento local estándar
- **Característica**: Archivo único sin fragmentación
- **Ventaja**: Simplicidad y rapidez de acceso

### 6.2 USB Split (Fragmentado)
- **Uso**: Medios con limitaciones de tamaño
- **Característica**: División en fragmentos de tamaño configurable
- **Ventaja**: Compatibilidad con sistemas de archivos FAT32, distribución en múltiples medios
- **Paralelización**: Escritura simultánea de fragmentos

### 6.3 Cloud (Simulado)
- **Estado**: Implementación placeholder
- **Propósito**: Preparación para integración con servicios cloud
- **Potencial**: AWS S3, Google Cloud Storage, Azure Blob

## 7. Instrucciones de Uso

### 7.1 Requisitos del Sistema

**Dependencias Python:**
```bash
pip install dask[complete]
```

**Requisitos de sistema:**
- Python 3.7+
- Memoria RAM: Mínimo 2GB, recomendado 4GB+
- Espacio en disco: Dependiente del tamaño de backup

### 7.2 Comandos de Backup

#### Backup Básico (HDD)
```bash
python cli.py backup --sources ./documentos ./fotos --output-dir ./respaldos --algo zip
```

#### Backup con Fragmentación (USB)
```bash
python cli.py backup --sources ./datos --output-dir ./usb_backup --algo gzip --dest-type usb_split --split-chunk-mb 100
```

#### Backup con Compresión BZIP2
```bash
python cli.py backup --sources ./proyecto --output-dir ./backup_comprimido --algo bzip2
```

### 7.3 Comandos de Restauración

#### Restauración Simple
```bash
python cli.py restore --source ./respaldos/backup_20250520_101010.zip --restore-to ./restaurado
```

#### Restauración desde Fragmentos
```bash
python cli.py restore --source ./fragmentos_dir --restore-to ./restaurado --is-split --original-base-filename backup_20250520_101010.tar.gz
```

### 7.4 Parámetros Avanzados

| Parámetro | Descripción | Valores |
|-----------|-------------|---------|
| `--algo` | Algoritmo de compresión | zip, gzip, bzip2 |
| `--dest-type` | Tipo de destino | hdd, usb_split, cloud |
| `--split-chunk-mb` | Tamaño de fragmento | Entero (MB) |
| `--is-split` | Indica restauración fragmentada | Flag |

### 7.5 Ejemplos de Casos de Uso

#### Caso 1: Backup Empresarial
```bash
# Backup de múltiples directorios con alta compresión
python cli.py backup --sources /home/datos /var/www /etc --output-dir /backup/servidor --algo bzip2
```

#### Caso 2: Backup para Transporte
```bash
# Fragmentación para DVD (4.7GB límite)
python cli.py backup --sources ./multimedia --output-dir ./dvd_backup --algo zip --dest-type usb_split --split-chunk-mb 4600
```

#### Caso 3: Backup Rápido
```bash
# Compresión rápida para backup frecuente
python cli.py backup --sources ./trabajo_diario --output-dir ./backup_rapido --algo zip
```

## 8. Rendimiento y Escalabilidad

### 8.1 Métricas de Rendimiento

**Paralelización efectiva en:**
- Exploración de directorios: Speedup lineal hasta N cores
- Escritura de fragmentos: Speedup dependiente del I/O
- Recolección de archivos: Mejora significativa con múltiples fuentes

**Cuellos de botella identificados:**
- Compresión: Secuencial por naturaleza de los algoritmos
- I/O de disco: Limitado por hardware de almacenamiento

### 8.2 Optimizaciones Implementadas

- **Workers = CPU cores**: Maximiza paralelización
- **Threads = 1**: Evita contención del GIL
- **Chunk loading**: Lectura eficiente para fragmentación
- **Fallback síncrono**: Garantiza funcionamiento en cualquier entorno

## 9. Conclusiones y Trabajo Futuro

### 9.1 Logros del Sistema

- **Paralelización efectiva**: Implementación robusta con Dask
- **Flexibilidad**: Soporte múltiples algoritmos y destinos
- **Robustez**: Manejo de errores y fallbacks
- **Usabilidad**: CLI intuitiva y bien documentada

### 9.2 Áreas de Mejora

- **Compresión paralela**: Investigar algoritmos paralelizables
- **Integración cloud real**: Implementar AWS S3, Azure, GCS
- **Encriptación**: Agregar seguridad a los respaldos
- **GUI**: Interfaz gráfica para usuarios menos técnicos
- **Incremental backup**: Respaldos diferenciales
- **Verificación de integridad**: Checksums y validación

### 9.3 Impacto Técnico

El sistema demuestra la viabilidad de usar Dask para paralelizar operaciones de backup tradicionalmente secuenciales, proporcionando una base sólida para sistemas de respaldo empresariales más complejos.