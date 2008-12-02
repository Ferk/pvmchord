#******************************************/
# Simulacion de Red P2P por el algoritmo de Chord
# Archivo: Makefile
#
# Prácticas de Ampliación de Sistemas Operativos
# 3º I. T. Informatica de Sistemas
# Universidad de Córdoba
#	
# Rafael Jesus García del Pino (i52gapir@uco.es)
# Fernando Carmona Varo (q22cavaf@uco.es)
#******************************************/

# Declaración de las variables y flags relativos a PVM,
# para arquitectura de 64 bits
PVM_ARCH= LINUX64
PVMDIR= $(HOME)/pvm3
XDIR= $(PVMDIR)/bin/$(PVM_ARCH)
CFLOPTS= -g -Wall -ggdb
CFLAGS= $(CFLOPTS) -I/usr/include/ $(ARCHCFLAGS)
PVMLIB= -lpvm3 -lgpvm3

CC= gcc

# Objetivos principales
all: principal nodo

# Compilación mediante comodines
%: %.c
	$(CC) $(CFLAGS) -o $@ $@.c $(PVMLIB)
	mv $@ $(XDIR)


# Testear el programa
test: all
	cd $(XDIR) && \
	./principal 


# Chequeo de sintaxis (Flymake)
check-syntax: 
	$(CC) $(CFLAGS) -o nul -S $(CHK_SOURCES)



