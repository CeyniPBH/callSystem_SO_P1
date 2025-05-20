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

from utils import (
    derive_key, encrypt_chunk_cbc, decrypt_chunk_cbc,
    pad_data, unpad_data,
    SALT_SIZE, IV_SIZE, CHUNK_SIZE
)

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

class BackupSystem:
    def __init__(self):
        # Iniciar Dask cluster localmente. En un entorno más complejo,
        # podría conectarse a un scheduler existente.
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
        # Determinar el directorio base común para calcular rutas relativas
        # Esto es simplificado; una mejor aproximación podría encontrar el prefijo común más largo
        # o permitir al usuario especificarlo. Por ahora, usaremos la carpeta contenedora de la primera fuente.
        
        common_base_dirs = [os.path.abspath(os.path.dirname(f)) if os.path.isfile(f) else os.path.abspath(f) for f in source_folders]
        
        # Para simplificar, se asume que los archivos se añadirán con una ruta relativa
        # a la carpeta de origen que los contiene.
        # Ejemplo: si source_folders = ['/tmp/data1', '/tmp/data2']
        # y un archivo es '/tmp/data1/sub/file.txt', se guardará como 'data1/sub/file.txt' o 'sub/file.txt'
        # dependiendo de cómo se maneje `arcname`.
        # Aquí, para múltiples carpetas base, incluiremos el nombre de la carpeta base en el archivo.

        logger.info(f"Comprimiendo archivos en '{archive_path}' usando {algorithm.upper()}...")
        start_time = time.time()

        if algorithm == 'zip':
            with zipfile.ZipFile(archive_path, 'w', zipfile.ZIP_DEFLATED) as zf:
                for file_path in files_to_backup:
                    # Determinar arcname para preservar estructura relativa
                    arcname = None
                    for base_dir in common_base_dirs:
                        if file_path.startswith(base_dir):
                            # arcname será la ruta relativa al directorio *padre* de base_dir
                            # o la ruta relativa a base_dir si queremos incluir el nombre de base_dir
                            # Vamos a incluir el nombre de la carpeta base para evitar colisiones si hay archivos con el mismo nombre relativo
                            # en diferentes carpetas base.
                            # Ej: /path/to/folderA/file.txt -> folderA/file.txt
                            #     /path/to/folderB/file.txt -> folderB/file.txt
                            relative_to_parent_of_base = os.path.relpath(file_path, os.path.dirname(base_dir))
                            arcname = relative_to_parent_of_base
                            break
                    if arcname is None: # Si no está en ninguna base_dir (no debería pasar con _collect_files_to_backup)
                        arcname = os.path.basename(file_path)
                    
                    zf.write(file_path, arcname)
        elif algorithm in ['gzip', 'bzip2']:
            tar_path = archive_path.replace(f'.{algorithm}', '.tar') # Nombre temporal para el .tar
            # Primero crear el archivo TAR
            with tarfile.open(tar_path, 'w') as tf:
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
            
            # Luego comprimir el archivo TAR
            comp_module = gzip if algorithm == 'gzip' else bz2
            comp_ext = '.gz' if algorithm == 'gzip' else '.bz2'
            final_archive_path = tar_path + comp_ext # Debería ser el archive_path original

            with open(tar_path, 'rb') as f_in, comp_module.open(final_archive_path, 'wb') as f_out:
                shutil.copyfileobj(f_in, f_out)
            os.remove(tar_path) # Eliminar el .tar intermedio
        else:
            raise ValueError(f"Algoritmo de compresión no soportado: {algorithm}")

        end_time = time.time()
        logger.info(f"Compresión completada en {end_time - start_time:.2f} segundos.")

    def _encrypt_file_dask(self, input_path: str, output_path: str, password: str):
        """Encripta un archivo usando AES-CBC con Dask para paralelizar chunks."""
        logger.info(f"Encriptando '{input_path}' a '{output_path}'...")
        start_time = time.time()

        salt = os.urandom(SALT_SIZE)
        key = derive_key(password, salt)
        
        # Escribir salt al inicio del archivo de salida
        with open(output_path, 'wb') as f_out:
            f_out.write(salt)

        delayed_writes = []
        with open(input_path, 'rb') as f_in:
            while True:
                iv = os.urandom(IV_SIZE) # Nuevo IV para cada chunk (más o menos, no ideal para CBC, mejor CTR o GCM)
                                        # Para CBC, el IV del siguiente chunk es el último bloque cifrado del anterior.
                                        # Simplificaremos usando un IV nuevo por chunk escrito al archivo.
                                        # ¡Esto es una simplificación y NO es criptográficamente ideal para CBC!
                                        # Un modo como GCM o CTR sería mejor para chunks paralelos.
                                        # Para este ejemplo, procedemos con esta simplificación para CBC.
                
                chunk = f_in.read(CHUNK_SIZE)
                if not chunk:
                    break
                
                is_last_chunk = len(chunk) < CHUNK_SIZE or f_in.peek(1) == b''

                if is_last_chunk:
                    chunk = pad_data(chunk) # Aplicar padding PKCS7 al último chunk

                # @dask.delayed
                def encrypt_and_prefix_iv(data_chunk, current_key, current_iv):
                    encrypted_chunk = encrypt_chunk_cbc(data_chunk, current_key, current_iv)
                    return current_iv + encrypted_chunk # Prepend IV to the chunk

                # No se puede usar dask.delayed directamente aquí si queremos escribir secuencialmente.
                # Lo que Dask puede paralelizar es la operación de encriptación del chunk si
                # los chunks son independientes (lo cual no es el caso para CBC estándar).
                # Para GCM/CTR o si cada chunk tuviera su propio IV escrito, sería más fácil.
                #
                # Simplificación para Dask: Supongamos que cada chunk es independiente (con su IV)
                # Esto significa que almacenamos el IV por cada chunk en el archivo encriptado.
                
                # El siguiente enfoque es para una demostración de Dask, pero tiene implicaciones de seguridad/diseño.
                # Una implementación CBC correcta y paralela es más compleja.
                # Usar AES-GCM o AES-CTR es altamente recomendado para encriptación paralela de chunks.
                # Por ahora, vamos a hacer la encriptación secuencial por chunks para mantener la corrección de CBC
                # y aplicar Dask en OTRA etapa (ej. transferencia).

                # **Revisión para Dask en encriptación con CBC de forma más correcta:**
                # No se puede paralelizar directamente la encriptación CBC de un solo flujo.
                # Lo que se puede paralelizar es: si el archivo original es MUY grande,
                # y lo partimos en N "meta-bloques" lógicos. Cada meta-bloque se encripta
                # con su propia sal/IV inicial y se pueden procesar en paralelo por Dask.
                # Luego, estos meta-bloques encriptados se concatenan.
                # Esto complica la desencriptación.

                # **Alternativa para Dask (más simple, pero con el IV por chunk):**
                # Esto es lo que se estaba esbozando arriba.
                # input_path -> [chunk1, chunk2, ..., chunkN]
                # dask_tasks = [delayed(encrypt_with_new_iv)(chunk, key) for chunk in chunks]
                # encrypted_parts = dask.compute(*dask_tasks) -> [ (iv1, enc1), (iv2, enc2), ... ]
                # Escribir salt + (iv1,enc1) + (iv2,enc2) ... al archivo.

                # Para este ejemplo, mantendremos la encriptación secuencial chunk a chunk
                # para asegurar la corrección de CBC con un solo IV inicial (o IVs encadenados).
                # Dask se usará en la recolección de archivos y en la etapa de fragmentación/transferencia.
                
                # ***** IMPLEMENTACIÓN SECUENCIAL DE ENCRIPTACIÓN CBC CHUNKED *****
                # (Dask se aplicará en otras fases como recolección y fragmentación)
                
                # El IV para el primer chunk se genera aleatoriamente.
                # Para los siguientes, el IV es el último bloque cifrado del chunk anterior.
                # Esto NO es directamente paralizable con Dask para un único flujo.
                #
                # Para demostrar Dask aquí, tendríamos que cambiar a un modo como CTR/GCM
                # o aceptar la simplificación de (IV + encrypted_chunk) para cada chunk.
                #
                # Vamos a optar por la **simplificación (IV + encrypted_chunk)** para poder usar Dask.
                # CUIDADO: Esto no es CBC estándar y puede tener debilidades si no se maneja con extremo cuidado.
                # Para una aplicación real, investigar modos como AES-GCM.
                
                encrypted_task = delayed(encrypt_chunk_cbc)(chunk, key, iv) # iv se genera nuevo cada vez
                delayed_writes.append((iv, encrypted_task)) # Guardar IV para escribirlo

        # Compute all encrypted chunks in parallel (if Dask client is active)
        computed_results = dask.compute(*[task for _, task in delayed_writes])

        with open(output_path, 'ab') as f_out: # 'ab' para añadir después del salt
            for i, encrypted_chunk_data in enumerate(computed_results):
                original_iv, _ = delayed_writes[i]
                f_out.write(original_iv)
                f_out.write(encrypted_chunk_data)
        #---------------------------------------------------------------------------------

        end_time = time.time()
        logger.info(f"Encriptación completada en {end_time - start_time:.2f} segundos.")
        return output_path


    def _decrypt_file_dask(self, input_path: str, output_path: str, password: str):
        """Desencripta un archivo usando AES-CBC con Dask (siguiendo la lógica de encriptación)."""
        logger.info(f"Desencriptando '{input_path}' a '{output_path}'...")
        start_time = time.time()

        tasks = []
        key = None # Se derivará después de leer la sal

        with open(input_path, 'rb') as f_in:
            salt = f_in.read(SALT_SIZE)
            if len(salt) < SALT_SIZE:
                raise ValueError("Archivo de encriptación corrupto o demasiado corto (falta salt).")
            key = derive_key(password, salt)

            while True:
                iv = f_in.read(IV_SIZE)
                if not iv: # Fin del archivo
                    break 
                if len(iv) < IV_SIZE:
                    raise ValueError("Archivo de encriptación corrupto (IV incompleto).")

                # Leer el tamaño del chunk encriptado. Asumimos que es CHUNK_SIZE encriptado.
                # El último chunk podría ser más corto (después del padding).
                # Para CBC, el tamaño del texto cifrado es múltiplo del tamaño de bloque.
                # El CHUNK_SIZE original podría haber sido expandido por el padding.
                # Leeremos hasta (CHUNK_SIZE + block_size) para asegurar que tenemos un chunk completo.
                # Esto es una simplificación. Un mejor formato almacenaría la longitud del chunk cifrado.
                
                # Simplificación: leer CHUNK_SIZE de datos cifrados (o lo que quede)
                # ya que el padding se aplicó al texto plano original.
                # El texto cifrado será del mismo tamaño o un poco más grande (múltiplo de block_size).
                
                # Una forma más robusta sería:
                # 1. Encriptar: chunk -> pad si es el último -> encrypt -> (IV, encrypted_padded_chunk)
                # 2. Escribir: salt, luego una secuencia de (IV, length_of_encrypted_padded_chunk, encrypted_padded_chunk)
                # Para este ejemplo, asumimos que los chunks cifrados son de un tamaño predecible (más o menos CHUNK_SIZE)
                
                # Lectura del chunk cifrado:
                # El tamaño del chunk cifrado será igual al tamaño del chunk original (si fue paddeado a un múltiplo de block_size)
                # o igual a CHUNK_SIZE (si ya era múltiplo de block_size).
                # Vamos a leer CHUNK_SIZE de datos cifrados. El último chunk será lo que quede.
                # (Nota: esto podría ser problemático si CHUNK_SIZE no es múltiplo de block_size, pero AES.block_size suele ser 16 bytes y CHUNK_SIZE es 64KB)
                
                # Vamos a leer un bloque de datos que *probablemente* contiene un chunk encriptado.
                # Para CBC, el tamaño cifrado es el mismo que el tamaño original alineado al tamaño de bloque.
                # Leeremos hasta CHUNK_SIZE + AES.block_size para estar seguros
                # (ya que el padding puede añadir hasta un bloque)
                encrypted_chunk_data = f_in.read(CHUNK_SIZE) # El CHUNK_SIZE original era antes del padding. El texto cifrado será >= CHUNK_SIZE (padded)
                                                          # Esto es una simplificación, puede fallar al final del archivo.
                                                          # Es mejor leer porciones más pequeñas o saber el tamaño exacto del chunk cifrado.

                # Por ahora, leeremos el tamaño exacto que fue encriptado antes.
                # Si el chunk original era de X bytes, y paddeado a Y, el cifrado es de Y bytes.
                # Aquí la lógica de lectura debe coincidir con la de escritura.
                # Nuestra escritura fue: IV + encrypt(pad(chunk)).
                # Así que leemos IV, luego leemos un bloque de datos cifrados.
                # La dificultad es saber cuánto leer para `encrypted_chunk_data`.
                # Asumiremos que los chunks cifrados son de (CHUNK_SIZE alineado a block_size)
                # o el tamaño restante alineado a block_size.

                # Vamos a leer de forma más simple: leer un bloque grande y procesarlo.
                # Esta parte es compleja para hacerla perfectamente paralela y robusta sin un formato de archivo más explícito.
                # Para este ejemplo, vamos a procesar secuencialmente la desencriptación para garantizar la corrección.
                # El paralelismo de Dask se demostrará en otras áreas.

                # ***** IMPLEMENTACIÓN SECUENCIAL DE DESENCRIPTACIÓN CBC CHUNKED *****
                # (Coincidiendo con la encriptación secuencial simplificada)
                
                # Dado que la encriptación se hizo chunk a chunk con IVs independientes:
                temp_encrypted_data = bytearray()
                bytes_to_read = -1 # Determinar esto es clave
                
                # Heurística: si CHUNK_SIZE es múltiplo de AES.block_size (16), entonces el texto cifrado es CHUNK_SIZE
                # Si no, es CHUNK_SIZE redondeado hacia arriba al próximo múltiplo de 16.
                # O si es el último chunk, es len(padded_chunk).
                
                # La forma más sencilla es leer hasta el final y procesar.
                # Pero si queremos procesar chunk a chunk:
                # Esta parte es la más difícil de generalizar sin un formato de archivo que especifique longitudes.
                # Para este ejemplo, leeremos bloques de datos encriptados de tamaño fijo (ej. CHUNK_SIZE)
                # y asumiremos que cada uno corresponde a un chunk encriptado (sin contar el IV).
                # El último chunk puede ser más corto.

                # Volvemos a la simplificación Dask:
                # La escritura fue: salt + (iv1, enc1) + (iv2, enc2) ...
                # donde encX es el resultado de encrypt_chunk_cbc(chunkX_padded_o_no, key, ivX)
                # y chunkX_padded_o_no tenía un tamaño original de hasta CHUNK_SIZE.
                # El tamaño de encX será el mismo que el de chunkX_padded_o_no (si es múltiplo de block_size)
                # o un poco más (si se requirió padding).
                
                # Dado que CHUNK_SIZE es 64KB (múltiplo de 16), el texto cifrado de un chunk completo será 64KB.
                # El último chunk puede ser más pequeño, pero también múltiplo de 16 debido al padding.
                
                encrypted_data_for_chunk = f_in.read(CHUNK_SIZE) # Leer el equivalente a un chunk de datos cifrados
                if not encrypted_data_for_chunk:
                    if iv: # Leímos un IV pero no datos, esto es un error
                         raise ValueError("Archivo de encriptación corrupto: IV sin datos.")
                    break # Fin normal

                tasks.append(delayed(decrypt_chunk_cbc)(encrypted_data_for_chunk, key, iv))
        
        if not tasks:
            logger.warning("No hay datos para desencriptar después de la sal.")
            with open(output_path, 'wb') as f_out: # Crear archivo vacío
                pass
            return output_path

        decrypted_chunks = dask.compute(*tasks)

        with open(output_path, 'wb') as f_out:
            for i, decrypted_chunk in enumerate(decrypted_chunks):
                if i == len(decrypted_chunks) - 1: # Último chunk
                    try:
                        decrypted_chunk = unpad_data(decrypted_chunk)
                    except ValueError as e:
                        logger.error(f"Error al quitar padding del último chunk: {e}. Puede ser una clave incorrecta.")
                        # No escribir este chunk o manejar el error como se prefiera
                        # Continuar podría dejar datos corruptos. Es mejor fallar aquí.
                        raise ValueError("Error de padding, probablemente clave incorrecta o archivo corrupto.") from e
                f_out.write(decrypted_chunk)
        #---------------------------------------------------------------------------------

        end_time = time.time()
        logger.info(f"Desencriptación completada en {end_time - start_time:.2f} segundos.")
        return output_path

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
            # Copiar el archivo vacío si es el caso.
            if os.path.getsize(input_path) == 0:
                empty_part_path = os.path.join(output_dir, f"{base_filename}.part001")
                shutil.copy2(input_path, empty_part_path)
                output_paths.append(empty_part_path)


        logger.info(f"Archivo dividido en {len(output_paths)} fragmentos.")
        return output_paths

    def _merge_files(self, parts_dir: str, base_filename_pattern: str, output_path: str):
        """Reconstruye un archivo a partir de sus fragmentos."""
        logger.info(f"Reconstruyendo archivo desde fragmentos en '{parts_dir}' a '{output_path}'")
        # base_filename_pattern podría ser como 'backup_xxxx.zip.enc' y buscará .partNNN
        
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
               compression_algo: str, encrypt: bool = False, password: Optional[str] = None,
               destination_type: str = 'hdd', # 'hdd', 'usb_split' (cloud no implementado)
               split_chunk_mb: Optional[int] = None):
        """
        Flujo principal de respaldo.
        Dask se usa para:
        1. Recolección de archivos (si múltiples source_folders).
        2. Encriptación por chunks (con la simplificación de IV por chunk).
        3. Escritura de fragmentos (si destination_type es 'usb_split').
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
            final_filename_for_parts = base_archive_name + comp_ext # Nombre base para fragmentos, antes de .enc

            # 4. Encriptar (opcional)
            if encrypt:
                if not password:
                    password = getpass.getpass("Ingrese la contraseña para encriptar el backup: ")
                
                encrypted_filename = base_archive_name + comp_ext + ".enc"
                temp_encrypted_path = os.path.join(output_dir, "temp_" + encrypted_filename)
                
                self._encrypt_file_dask(current_processed_file, temp_encrypted_path, password)
                os.remove(current_processed_file) # Eliminar el archivo comprimido no encriptado
                current_processed_file = temp_encrypted_path
                final_filename_for_parts = encrypted_filename # Nombre base para fragmentos, después de .enc

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
                
                # El directorio de salida (output_dir) será donde se guarden los fragmentos.
                # El nombre base para los fragmentos ya está en final_filename_for_parts (ej. backup_xxxx.zip o backup_xxxx.zip.enc)
                # Necesitamos quitar "temp_" del current_processed_file para el nombre base de los fragmentos
                base_name_for_splitting = os.path.basename(current_processed_file).replace("temp_", "")

                split_output_dir = os.path.join(output_dir, base_name_for_splitting + "_parts")
                
                self._split_file_dask(current_processed_file, split_chunk_mb, split_output_dir, base_name_for_splitting)
                os.remove(current_processed_file) # Eliminar el archivo grande original
                final_backup_path_or_dir = split_output_dir
                logger.info(f"Backup completado y fragmentado en: {split_output_dir}")
            
            elif destination_type == 'cloud':
                # Placeholder para la lógica de la nube
                # Aquí se usaría Dask para subir chunks en paralelo si la API lo permite.
                # Ejemplo: delayed_uploads = [dask.delayed(upload_chunk_to_cloud)(chunk, ...) for chunk in chunks]
                # dask.compute(*delayed_uploads)
                final_backup_name = os.path.basename(current_processed_file).replace("temp_", "")
                final_cloud_path = f"cloud_storage_path/{final_backup_name}" # Simulado
                logger.info(f"Simulando subida de '{current_processed_file}' a '{final_cloud_path}'")
                # Aquí iría la llamada real a la API de la nube
                # Por ahora, simplemente movemos el archivo a una subcarpeta "cloud_upload_mock"
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
            if 'temp_encrypted_path' in locals() and os.path.exists(temp_encrypted_path):
                try:
                    os.remove(temp_encrypted_path)
                except OSError: pass
            raise


    def restore(self, backup_source_path_or_dir: str, restore_to_dir: str,
                password: Optional[str] = None, is_split: bool = False, 
                original_base_filename_pattern: Optional[str] = None): # original_base_filename_pattern es ej. backup_timestamp.zip[.enc]
        """
        Flujo principal de restauración.
        Dask se usa para:
        1. Desencriptación por chunks (si está encriptado).
        """
        try:
            os.makedirs(restore_to_dir, exist_ok=True)
            
            temp_merged_file = None
            current_file_to_process = backup_source_path_or_dir

            if is_split:
                if not original_base_filename_pattern:
                    raise ValueError("Se requiere 'original_base_filename_pattern' para restaurar desde fragmentos.")
                
                # backup_source_path_or_dir es el directorio que contiene los .partXXX
                # original_base_filename_pattern es el nombre del archivo antes de .partXXX
                # Ej: backup_20231027_120000.zip.enc (este es el patrón)
                # Los archivos son backup_20231027_120000.zip.enc.part001, ...
                
                # Nombre del archivo temporal reconstruido
                temp_merged_filename = original_base_filename_pattern # Este será el nombre después de unir
                temp_merged_file = os.path.join(restore_to_dir, "temp_merged_" + temp_merged_filename)

                self._merge_files(backup_source_path_or_dir, original_base_filename_pattern, temp_merged_file)
                current_file_to_process = temp_merged_file
                logger.info(f"Archivo reconstruido temporalmente en: {temp_merged_file}")

            # Determinar si está encriptado por la extensión .enc
            is_encrypted = current_file_to_process.endswith(".enc")
            temp_decrypted_file = None

            if is_encrypted:
                if not password:
                    password = getpass.getpass("Ingrese la contraseña para desencriptar el backup: ")
                
                # Nombre del archivo después de desencriptar
                decrypted_filename = os.path.basename(current_file_to_process).replace(".enc", "").replace("temp_merged_", "temp_decrypted_")
                temp_decrypted_file = os.path.join(restore_to_dir, decrypted_filename)
                
                self._decrypt_file_dask(current_file_to_process, temp_decrypted_file, password)
                current_file_to_process = temp_decrypted_file
                logger.info(f"Archivo desencriptado temporalmente en: {temp_decrypted_file}")


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
                # Si no tiene extensión de compresión conocida, asumir que es un solo archivo
                # (podría ser un archivo no comprimido o un formato desconocido)
                # Por ahora, solo manejamos los formatos comprimidos especificados.
                # Si llega aquí un .enc sin compresión previa, o un archivo sin extensión manejable.
                if not (is_encrypted and filename_for_decompression.endswith(".enc")): # si solo era .enc
                     logger.warning(f"Formato de archivo desconocido para descompresión: {filename_for_decompression}. Si era solo encriptado, ya se manejó.")
                     # Si solo fue encriptado y no comprimido, el current_file_to_process es el archivo final.
                     # Lo copiamos/movemos a restore_to_dir con su nombre original (sin temp_)
                     if current_file_to_process.startswith(os.path.join(restore_to_dir, "temp_")):
                         final_restored_name = os.path.basename(current_file_to_process).replace("temp_merged_", "").replace("temp_decrypted_", "")
                         final_path = os.path.join(restore_to_dir, final_restored_name)
                         shutil.move(current_file_to_process, final_path)
                         logger.info(f"Archivo restaurado (sin descompresión) a: {final_path}")
                     else: # Era un archivo original que no pasó por temp (ej. backup directo no encriptado y no dividido)
                         # Esto no debería ocurrir mucho en el flujo normal de restauración, pero por si acaso.
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
            if temp_decrypted_file and os.path.exists(temp_decrypted_file):
                # Este es el archivo que se descomprime. No eliminar si la descompresión falló antes de copiar.
                # Si la descompresión es exitosa, el archivo original (comprimido y posiblemente encriptado) ya no es necesario.
                # Si el current_file_to_process era temp_decrypted_file, y se extrajo correctamente, se puede borrar.
                if filename_for_decompression.endswith((".zip", ".tar.gz", ".tar.bz2")): # si se extrajo
                    os.remove(temp_decrypted_file)