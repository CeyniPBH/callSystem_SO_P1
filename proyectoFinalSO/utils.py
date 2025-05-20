# utils.py
import os
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.primitives.kdf.pbkdf2 import PBKDF2HMAC
from cryptography.hazmat.primitives import hashes, padding
from cryptography.hazmat.backends import default_backend

SALT_SIZE = 16
KEY_SIZE = 32  # AES-256
IV_SIZE = 16   # AES block size / IV size
CHUNK_SIZE = 64 * 1024  # 64KB chunks for processing

backend = default_backend()

def derive_key(password: str, salt: bytes) -> bytes:
    """Deriva una clave de encriptación desde una contraseña y una sal."""
    kdf = PBKDF2HMAC(
        algorithm=hashes.SHA256(),
        length=KEY_SIZE,
        salt=salt,
        iterations=100000,
        backend=backend
    )
    return kdf.derive(password.encode())

def encrypt_chunk_cbc(chunk: bytes, key: bytes, iv: bytes) -> bytes:
    """Encripta un solo chunk usando AES-CBC. Necesita padding manual para el último."""
    cipher = Cipher(algorithms.AES(key), modes.CBC(iv), backend=backend)
    encryptor = cipher.encryptor()
    # El padding se debe manejar fuera, antes de llamar a esta función si es el último chunk
    return encryptor.update(chunk) + encryptor.finalize()

def decrypt_chunk_cbc(chunk: bytes, key: bytes, iv: bytes) -> bytes:
    """Desencripta un solo chunk usando AES-CBC. El unpadding se debe manejar fuera."""
    cipher = Cipher(algorithms.AES(key), modes.CBC(iv), backend=backend)
    decryptor = cipher.decryptor()
    return decryptor.update(chunk) + decryptor.finalize()

def pad_data(data: bytes) -> bytes:
    """Aplica padding PKCS7."""
    padder = padding.PKCS7(algorithms.AES.block_size).padder()
    return padder.update(data) + padder.finalize()

def unpad_data(data: bytes) -> bytes:
    """Remueve padding PKCS7."""
    unpadder = padding.PKCS7(algorithms.AES.block_size).unpadder()
    return unpadder.update(data) + unpadder.finalize()