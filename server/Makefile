# Universidad del Valle de Guatemala
# Departamento de Computación
# Sistemas Operativos - CC3064
#
# Autores:
#  - Angie Vela, 23764
#  - Adrián González, 23152
#  - Paula De León, 23202
#
# Archivo: server/Makefile 
# Descripción: Reglas de compilación del servidor de chat. Compila los
#              módulos en C con soporte para pthreads y genera el binario
#              ejecutable del servidor.

CC      = gcc
CFLAGS  = -Wall -Wextra -pedantic -pthread -g
COMMON  = ../common

SRV_SRCS = main.c server.c client_handler.c user_manager.c \
           $(COMMON)/protocol.c $(COMMON)/utils.c
SRV_OBJS = $(SRV_SRCS:.c=.o)
SRV_BIN  = server

server: $(SRV_OBJS)
	$(CC) $(CFLAGS) -o $(SRV_BIN) $(SRV_OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(SRV_OBJS) $(SRV_BIN)

.PHONY: clean