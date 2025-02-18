# Makefile para el programa de compresion LZW

CC = g++
CFLAGS = -std=c++11 -Wall

# Archivos fuente y objeto
SOURCES = main.cpp lzw.cpp
OBJECTS = $(SOURCES:.cpp=.o)
EXECUTABLE = lzw

# Regla principal
all: $(EXECUTABLE)

# Regla para el ejecutable
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

# Regla genérica para objetos
%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

# Dependencias
main.o: main.cpp lzw.h
lzw.o: lzw.cpp lzw.h

# Limpiar archivos generados
clean:
	rm -f $(OBJECTS) $(EXECUTABLE)

# Regla para instalar el programa
install: $(EXECUTABLE)
	mkdir -p $(DESTDIR)/usr/local/bin
	cp $(EXECUTABLE) $(DESTDIR)/usr/local/bin/

# Regla para desinstalar el programa
uninstall:
	rm -f $(DESTDIR)/usr/local/bin/$(EXECUTABLE)