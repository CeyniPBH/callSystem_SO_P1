
# Sistema de Respaldo Seguro con Dask

Este proyecto proporciona una herramienta de línea de comandos (`cli.py`) para crear y restaurar respaldos comprimidos y opcionalmente encriptados, utilizando procesamiento distribuido con Dask.

## 🧾 Archivos del Proyecto

- `backup_system.py`: Módulo principal con la lógica de respaldo y restauración.
- `cli.py`: Interfaz de línea de comandos para interactuar con el sistema.

## ⚙️ Requisitos e Instalación

Antes de ejecutar, asegúrate de tener Python 3 instalado y luego instala las siguientes bibliotecas:

```bash
pip install dask distributed
````

## 🚀 Instrucciones de Uso

Guarda ambos archivos en el mismo directorio:
`backup_system.py`, `cli.py`

### 📦 Crear un backup ZIP de una carpeta (a un disco externo)

```bash
python cli.py backup --sources "/ruta/a/mis_documentos" --output-dir "/mnt/disco_externo/backups" --algo zip --dest-type hdd
```

### ♻️ Restaurar un backup ZIP

```bash
python cli.py restore --source "/mnt/disco_externo/backups/backup_20231027_110000.zip" --restore-to "/ruta/de/restauracion"
```

### 📦 Crear un backup TAR.GZ de múltiples carpetas, dividido para USBs

```bash
python cli.py backup --sources "/ruta/a/carpeta1" "/ruta/a/carpeta2" --output-dir "/tmp/backup_staging" --algo gzip --dest-type usb_split --split-chunk-mb 100
```

💡 Luego copia el contenido de `/tmp/backup_staging/backup_timestamp.tar.gz.enc_parts/` a los USBs correspondientes.

### ♻️ Restaurar un backup fragmentado desde USB
(Suponiendo que los fragmentos están en "/ruta/usb/backup_20250522_192654.tar.gz_parts" y el nombre original de los backups era "backup_20250522_192654.tar.gz")
```bash
python cli.py restore --source "/ruta/usb/backup_20250522_192654.tar.gz_parts" --restore-to "/ruta/de/restauracion" --is-split --original-base-filename "backup_20250522_192654.tar.gz"
```
