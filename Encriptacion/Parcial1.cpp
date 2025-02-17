#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <sys/stat.h>

#define BUFFER_SIZE 1024
#define XOR_KEY 0x5A  // Clave para encriptación y desencriptación

void encrypt_decrypt(const char *input_file) {
    // Crear una copia temporal del archivo que se va a encriptar o desencriptar
    std::string temp_file = "archivo_encriptar_desencriptar.txt";
    std::string command_cp = "cp " + std::string(input_file) + " " + temp_file;
    system(command_cp.c_str());

    // crear un archivo temporal de salida para que sea movido al archivo original
    std::string processed_file = "archivo_encriptado_desencriptado.txt";

    int fd_in = open(temp_file.c_str(), O_RDONLY);
    if (fd_in < 0) {
        perror("Error al abrir el archivo de entrada");
        return;
    }

    int fd_out = open(processed_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd_out < 0) {
        perror("Error al abrir el archivo de salida");
        close(fd_in);
        return;
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    while ((bytes_read = read(fd_in, buffer, BUFFER_SIZE)) > 0) {
        for (ssize_t i = 0; i < bytes_read; i++) {
            buffer[i] ^= XOR_KEY;  // Aplicar XOR para encriptar/desencriptar
        }
        write(fd_out, buffer, bytes_read);
    }

    close(fd_in);
    close(fd_out);

    // Reemplazar el archivo original con el archivo que ya fue encriptado o desencriptado segun lo que elija el usuario
    std::string command_mv = "mv " + processed_file + " " + std::string(input_file);
    system(command_mv.c_str());

    // Eliminar la copia temporal del archivo original
    std::string command_rm = "rm " + temp_file;
    system(command_rm.c_str());

    std::cout << "Archivo procesado y reemplazado: " << input_file << "\n";
}

void show_help() {
    std::cout << "Uso: Encriptador [opciones] <archivo>\n"
              << "Opciones:\n"
              << "  -h, --help       Muestra este mensaje\n"
              << "  -v, --version    Muestra la versión del programa\n"
              << "  -e <archivo>     Encripta el archivo (modifica el original)\n"
              << "  -d <archivo>     Desencripta el archivo (modifica el original)\n";
}

void show_version() {
    std::cout << "Encriptador v1.1\n";
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        show_help();
        return 1;
    }

    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        show_help();
    } else if (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0) {
        show_version();
    } else if ((strcmp(argv[1], "-e") == 0 || strcmp(argv[1], "--encrypt") == 0) && argc == 3) {
        encrypt_decrypt(argv[2]);
    } else if ((strcmp(argv[1], "-d") == 0 || strcmp(argv[1], "--decrypt") == 0) && argc == 3) {
        encrypt_decrypt(argv[2]);
    } else {
        std::cerr << "Opción no reconocida. Use -h para ayuda.\n";
        return 1;
    }

    return 0;
}
