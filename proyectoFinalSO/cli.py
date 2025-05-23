# cli.py
import argparse
import os
import sys
from backup_system import BackupSystem, logger # Import logger para que se configure

def main():
    parser = argparse.ArgumentParser(description="Sistema de Respaldo Seguro con Dask.")
    subparsers = parser.add_subparsers(dest="command", required=True, help="Comando a ejecutar")

    # Subcomando Backup
    backup_parser = subparsers.add_parser("backup", help="Crear un nuevo respaldo.")
    backup_parser.add_argument("--sources", nargs="+", required=True, help="Carpetas de origen a respaldar.")
    backup_parser.add_argument("--output-dir", required=True, help="Directorio donde se guardará el respaldo.")
    backup_parser.add_argument("--algo", choices=['zip', 'gzip', 'bzip2'], default='zip', help="Algoritmo de compresión.")
    backup_parser.add_argument("--dest-type", choices=['hdd', 'usb_split', 'cloud'], default='hdd',
                               help="Tipo de destino de almacenamiento.")
    backup_parser.add_argument("--split-chunk-mb", type=int, help="Tamaño de fragmento en MB para 'usb_split'.")

    # Subcomando Restore
    restore_parser = subparsers.add_parser("restore", help="Restaurar desde un respaldo.")
    restore_parser.add_argument("--source", required=True, help="Archivo de respaldo o directorio de fragmentos a restaurar.")
    restore_parser.add_argument("--restore-to", required=True, help="Directorio donde se restaurarán los archivos.")
    restore_parser.add_argument("--is-split", action="store_true", help="Indica si el origen es un directorio de fragmentos.")
    restore_parser.add_argument("--original-base-filename", help="Nombre base original del archivo fragmentado (ej: backup_xxxx.zip o backup_xxxx.tar.gz). Necesario si --is-split.")


    args = parser.parse_args()
    
    # Validaciones adicionales
    if args.command == "backup":
        if args.dest_type == "usb_split" and not args.split_chunk_mb:
            parser.error("--split-chunk-mb es requerido cuando --dest-type es 'usb_split'.")
        for src_path in args.sources:
            if not os.path.exists(src_path):
                parser.error(f"La ruta de origen '{src_path}' no existe.")
        if not os.path.isdir(args.output_dir) and args.dest_type != "cloud": # Para cloud, output_dir podría ser un bucket o similar.
             try:
                os.makedirs(args.output_dir, exist_ok=True)
             except OSError as e:
                parser.error(f"No se pudo crear el directorio de salida '{args.output_dir}': {e}")


    if args.command == "restore":
        if args.is_split and not args.original_base_filename:
            parser.error("--original-base-filename es requerido cuando --is-split está activado.")
        if not os.path.exists(args.source):
            parser.error(f"La ruta de origen del respaldo '{args.source}' no existe.")
        if not os.path.isdir(args.restore_to):
            try:
                os.makedirs(args.restore_to, exist_ok=True)
            except OSError as e:
                parser.error(f"No se pudo crear el directorio de restauración '{args.restore_to}': {e}")


    system = BackupSystem() # Esto inicializa Dask

    try:
        if args.command == "backup":
            password_for_backup = None
            system.backup(
                source_folders=args.sources,
                output_dir=args.output_dir,
                compression_algo=args.algo,
                destination_type=args.dest_type,
                split_chunk_mb=args.split_chunk_mb
            )
        elif args.command == "restore":
            password_for_restore = None
            # Se pedirá interactivamente dentro de la función de restore si es necesario
            system.restore(
                backup_source_path_or_dir=args.source,
                restore_to_dir=args.restore_to,
                is_split=args.is_split,
                original_base_filename_pattern=args.original_base_filename
            )
    except Exception as e:
        logger.error(f"Operación fallida: {e}")
        # system.__del__() es llamado automáticamente al salir del scope o al finalizar el programa
        sys.exit(1)
    finally:
        # Asegurar que Dask se cierre correctamente
        del system


if __name__ == "__main__":
    main()