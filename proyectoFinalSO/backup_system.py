# backup_system.py
import os
import shutil
import time
import zipfile
import tarfile
import gzip
import bz2
import logging
from typing import List, Optional, Callable
from dask.distributed import Client, LocalCluster
import dask
from dask import delayed
import getpass

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

class BackupSystem:
    def __init__(self):
        # Iniciar Dask cluster localmente.
        try:
            self.cluster = LocalCluster(n_workers=os.cpu_count(), threads_per_worker=1)
            self.client = Client(self.cluster)
            logger.info(f"Dask client conectado: {self.client.dashboard_link}")
        except Exception as e:
            logger.warning(f"No se pudo iniciar Dask LocalCluster, usando Dask en modo síncrono: {e}")
            # Fallback a modo síncrono si el cluster no inicia (ej. en algunos entornos restringidos)
            dask.config.set(scheduler='synchronous')
            self.client = None # No hay cliente si no hay cluster
            self.cluster = None

    def __del__(self):
        if self.client:
            self.client.close()
        if self.cluster:
            self.cluster.close()
        logger.info("Dask client y cluster cerrados.")


    def _collect_files_to_backup(self, source_folders: List[str]) -> List[str]:
        """
        Recopila todos los archivos de las carpetas de origen, incluyendo subcarpetas.
        Usa Dask para paralelizar la exploración de múltiples carpetas raíz.
        """
        logger.info(f"Recolectando archivos de: {source_folders}")
        
        @delayed
        def get_files_in_folder(folder_path: str) -> List[str]:
            filepaths = []
            for root, _, files in os.walk(folder_path):
                for file in files:
                    filepaths.append(os.path.join(root, file))
            return filepaths

        delayed_results = [get_files_in_folder(folder) for folder in source_folders]
        
        all_files_nested = dask.compute(*delayed_results)
        
        # Aplanar la lista de listas
        all_files = [item for sublist in all_files_nested for item in sublist]
        
        logger.info(f"Se encontraron {len(all_files)} archivos para respaldar.")
        return all_files

    def _compress_files(self, files_to_backup: List[str], archive_path: str, algorithm: str, source_folders: List[str]):
        """
        Comprime los archivos en un único archivo de backup.
        El source_folders es necesario para preservar la estructura relativa de directorios en el archivo.
        """
        
        common_base_dirs = [os.path.abspath(os.path.dirname(f)) if os.path.isfile(f) else os.path.abspath(f) for f in source_folders]

        logger.info(f"Comprimiendo archivos en '{archive_path}' usando {algorithm.upper()}...")
        start_time = time.time()

        if algorithm == 'zip':
            with zipfile.ZipFile(archive_path, 'w', zipfile.ZIP_DEFLATED) as zf:
                for file_path in files_to_backup:
                    # Determinar arcname para preservar estructura relativa
                    arcname = None
                    for base_dir in common_base_dirs:
                        if file_path.startswith(base_dir):
                            relative_to_parent_of_base = os.path.relpath(file_path, os.path.dirname(base_dir))
                            arcname = relative_to_parent_of_base
                            break
                    if arcname is None: # Si no está en ninguna base_dir (no debería pasar con _collect_files_to_backup)
                        arcname = os.path.basename(file_path)
                    
                    zf.write(file_path, arcname)
        elif algorithm in ['gzip', 'bzip2']:
            # archive_path es el nombre deseado para el archivo final comprimido (e.g., "backup.tar.gz")
            
            # Determinar el path para el archivo .tar intermedio
            if algorithm == 'gzip':
                if not archive_path.endswith(".tar.gz"):
                    raise ValueError(f"Para gzip, archive_path ('{archive_path}') debe terminar en .tar.gz")
                intermediate_tar_path = archive_path[:-3]
                comp_module = gzip
            elif algorithm == 'bzip2':
                if not archive_path.endswith(".tar.bz2"):
                    raise ValueError(f"Para bzip2, archive_path ('{archive_path}') debe terminar en .tar.bz2")
                intermediate_tar_path = archive_path[:-4]
                comp_module = bz2
            else:
                raise ValueError(f"Algoritmo no soportado para tarring: {algorithm}")

            logger.debug(f"Creando archivo TAR intermedio en: {intermediate_tar_path}")
            # Primero crear el archivo TAR
            with tarfile.open(intermediate_tar_path, 'w') as tf:
                for file_path in files_to_backup:
                    arcname = None
                    for base_dir in common_base_dirs:
                        if file_path.startswith(base_dir):
                            relative_to_parent_of_base = os.path.relpath(file_path, os.path.dirname(base_dir))
                            arcname = relative_to_parent_of_base
                            break
                    if arcname is None:
                        arcname = os.path.basename(file_path)
                    tf.add(file_path, arcname=arcname)
            
            logger.debug(f"Comprimiendo '{intermediate_tar_path}' a '{archive_path}'")
            # Luego comprimir el archivo TAR al 'archive_path' final
            with open(intermediate_tar_path, 'rb') as f_in, comp_module.open(archive_path, 'wb') as f_out:
                shutil.copyfileobj(f_in, f_out)
            
            os.remove(intermediate_tar_path) # Eliminar el .tar intermedio
            logger.debug(f"Archivo TAR intermedio '{intermediate_tar_path}' eliminado.")
        else:
            raise ValueError(f"Algoritmo de compresión no soportado: {algorithm}")

        end_time = time.time()
        logger.info(f"Compresión completada en {end_time - start_time:.2f} segundos.")

    def _split_file_dask(self, input_path: str, chunk_size_mb: int, output_dir: str, base_filename: str) -> List[str]:
        """
        Divide un archivo en fragmentos de tamaño configurable.
        Usa Dask para escribir los fragmentos en paralelo.
        """
        logger.info(f"Dividiendo '{input_path}' en fragmentos de {chunk_size_mb}MB en '{output_dir}'")
        os.makedirs(output_dir, exist_ok=True)
        
        chunk_size_bytes = chunk_size_mb * 1024 * 1024
        part_num = 1
        tasks = []
        output_paths = []

        @delayed
        def write_chunk(data_chunk: bytes, part_path: str):
            with open(part_path, 'wb') as f_part:
                f_part.write(data_chunk)
            logger.info(f"Fragmento '{part_path}' escrito.")
            return part_path

        with open(input_path, 'rb') as f_in:
            while True:
                chunk = f_in.read(chunk_size_bytes)
                if not chunk:
                    break
                
                part_filename = f"{base_filename}.part{part_num:03d}"
                part_path = os.path.join(output_dir, part_filename)
                output_paths.append(part_path)
                
                tasks.append(write_chunk(chunk, part_path))
                part_num += 1
        
        if tasks:
            dask.compute(*tasks) # Ejecutar todas las escrituras de fragmentos en paralelo
        else:
            logger.warning(f"El archivo '{input_path}' está vacío, no se generaron fragmentos.")
            if os.path.getsize(input_path) == 0:
                empty_part_path = os.path.join(output_dir, f"{base_filename}.part001")
                shutil.copy2(input_path, empty_part_path)
                output_paths.append(empty_part_path)


        logger.info(f"Archivo dividido en {len(output_paths)} fragmentos.")
        return output_paths

    def _merge_files(self, parts_dir: str, base_filename_pattern: str, output_path: str):
        """Reconstruye un archivo a partir de sus fragmentos."""
        logger.info(f"Reconstruyendo archivo desde fragmentos en '{parts_dir}' a '{output_path}'")
        # base_filename_pattern podría ser como 'backup_xxxx.zip' y buscará .partNNN
        
        part_files = sorted([
            os.path.join(parts_dir, f) for f in os.listdir(parts_dir) 
            if f.startswith(base_filename_pattern) and '.part' in f
        ])

        if not part_files:
            raise FileNotFoundError(f"No se encontraron fragmentos para '{base_filename_pattern}' en '{parts_dir}'")

        logger.info(f"Fragmentos encontrados: {part_files}")

        with open(output_path, 'wb') as f_out:
            for part_file in part_files:
                with open(part_file, 'rb') as f_in:
                    shutil.copyfileobj(f_in, f_out)
        logger.info("Archivo reconstruido exitosamente.")


    def backup(self, source_folders: List[str], output_dir: str,
               compression_algo: str,
               destination_type: str = 'hdd', # 'hdd', 'usb_split', 'cloud' (no implementado del todo)
               split_chunk_mb: Optional[int] = None):
        """
        Flujo principal de respaldo.
        Dask se usa para:
        1. Recolección de archivos (si múltiples source_folders).
        2. Escritura de fragmentos (si destination_type es 'usb_split').
        """
        try:
            # 1. Recolectar archivos
            files_to_backup = self._collect_files_to_backup(source_folders)
            if not files_to_backup:
                logger.warning("No se encontraron archivos para respaldar. Abortando.")
                return

            # 2. Preparar nombre de archivo base
            timestamp = time.strftime("%Y%m%d_%H%M%S")
            base_archive_name = f"backup_{timestamp}"
            
            # Determinar extensión de compresión
            if compression_algo == 'zip':
                comp_ext = '.zip'
            elif compression_algo == 'gzip':
                comp_ext = '.tar.gz'
            elif compression_algo == 'bzip2':
                comp_ext = '.tar.bz2'
            else:
                raise ValueError(f"Algoritmo de compresión '{compression_algo}' no soportado.")

            temp_archive_filename = base_archive_name + comp_ext
            temp_archive_path = os.path.join(output_dir, "temp_" + temp_archive_filename) # Archivo comprimido temporal
            os.makedirs(output_dir, exist_ok=True) # Asegurar que el directorio de salida exista

            # 3. Comprimir
            self._compress_files(files_to_backup, temp_archive_path, compression_algo, source_folders)

            current_processed_file = temp_archive_path
            final_filename_for_parts = base_archive_name + comp_ext # Nombre base para fragmentos

            # 5. Almacenar / Fragmentar
            final_backup_path_or_dir = ""

            if destination_type == 'hdd':
                final_backup_name = os.path.basename(current_processed_file).replace("temp_", "")
                final_backup_path = os.path.join(output_dir, final_backup_name)
                shutil.move(current_processed_file, final_backup_path)
                final_backup_path_or_dir = final_backup_path
                logger.info(f"Backup completado y guardado en: {final_backup_path}")

            elif destination_type == 'usb_split':
                if not split_chunk_mb:
                    raise ValueError("Se debe especificar --split-chunk-mb para destination_type 'usb_split'.")
                
                base_name_for_splitting = os.path.basename(current_processed_file).replace("temp_", "")

                split_output_dir = os.path.join(output_dir, base_name_for_splitting + "_parts")
                
                self._split_file_dask(current_processed_file, split_chunk_mb, split_output_dir, base_name_for_splitting)
                os.remove(current_processed_file) # Eliminar el archivo grande original
                final_backup_path_or_dir = split_output_dir
                logger.info(f"Backup completado y fragmentado en: {split_output_dir}")
            
            elif destination_type == 'cloud':
                # Placeholder para la lógica de la nube. Por ahora, simplemente movemos el archivo a una subcarpeta "cloud_upload_mock"
                # Para simular que lo estamos subiendo a la nube
                final_backup_name = os.path.basename(current_processed_file).replace("temp_", "")
                final_cloud_path = f"cloud_storage_path/{final_backup_name}" # Simulado
                logger.info(f"Simulando subida de '{current_processed_file}' a '{final_cloud_path}'")
                mock_cloud_dir = os.path.join(output_dir, "cloud_upload_mock")
                os.makedirs(mock_cloud_dir, exist_ok=True)
                final_mock_path = os.path.join(mock_cloud_dir, final_backup_name)
                shutil.move(current_processed_file, final_mock_path)
                logger.info(f"Backup (simulado para nube) guardado en: {final_mock_path}")
                final_backup_path_or_dir = final_mock_path


            else:
                os.remove(current_processed_file) # Limpiar archivo temporal si no se manejó el destino
                raise ValueError(f"Tipo de destino no soportado: {destination_type}")

        except Exception as e:
            logger.error(f"Error durante el proceso de backup: {e}", exc_info=True)
            # Limpieza de archivos temporales si existen
            if 'temp_archive_path' in locals() and os.path.exists(temp_archive_path):
                try:
                    os.remove(temp_archive_path)
                except OSError: pass # puede estar bloqueado
            raise


    def restore(self, backup_source_path_or_dir: str, restore_to_dir: str,
                is_split: bool = False, 
                original_base_filename_pattern: Optional[str] = None): # original_base_filename_pattern es ej. backup_timestamp.zip
        try:
            os.makedirs(restore_to_dir, exist_ok=True)
            
            temp_merged_file = None
            current_file_to_process = backup_source_path_or_dir

            if is_split:
                if not original_base_filename_pattern:
                    raise ValueError("Se requiere 'original_base_filename_pattern' para restaurar desde fragmentos.")
                
                # Nombre del archivo temporal reconstruido
                temp_merged_filename = original_base_filename_pattern # Este será el nombre después de unir
                temp_merged_file = os.path.join(restore_to_dir, "temp_merged_" + temp_merged_filename)

                self._merge_files(backup_source_path_or_dir, original_base_filename_pattern, temp_merged_file)
                current_file_to_process = temp_merged_file
                logger.info(f"Archivo reconstruido temporalmente en: {temp_merged_file}")

            # Determinar algoritmo de descompresión por extensión
            filename_for_decompression = os.path.basename(current_file_to_process)
            
            logger.info(f"Descomprimiendo '{current_file_to_process}' en '{restore_to_dir}'...")
            start_time = time.time()

            if filename_for_decompression.endswith(".zip"):
                with zipfile.ZipFile(current_file_to_process, 'r') as zf:
                    zf.extractall(restore_to_dir)
            elif filename_for_decompression.endswith(".tar.gz"):
                with tarfile.open(current_file_to_process, 'r:gz') as tf:
                    tf.extractall(restore_to_dir)
            elif filename_for_decompression.endswith(".tar.bz2"):
                with tarfile.open(current_file_to_process, 'r:bz2') as tf:
                    tf.extractall(restore_to_dir)
            else:
                logger.warning(f"Formato de archivo desconocido para descompresión: {filename_for_decompression}.")
                shutil.copy2(current_file_to_process, os.path.join(restore_to_dir, os.path.basename(current_file_to_process)))


            end_time = time.time()
            logger.info(f"Restauración completada en {end_time - start_time:.2f} segundos. Archivos en '{restore_to_dir}'.")

        except Exception as e:
            logger.error(f"Error durante el proceso de restauración: {e}", exc_info=True)
            raise
        finally:
            # Limpieza de archivos temporales
            if temp_merged_file and os.path.exists(temp_merged_file):
                os.remove(temp_merged_file)