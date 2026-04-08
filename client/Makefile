# Universidad del Valle de Guatemala
# Departamento de Computación
# Sistemas Operativos - CC3064
#
# Autores:
#  - Angie Vela, 23764
#  - Adrián González, 23152
#  - Paula De León, 23202
#
# Archivo: client/Makefile
# Descripción: Reglas de compilación del cliente de chat. Compila los
#              módulos en C con soporte para pthreads y genera el binario
#              ejecutable del cliente.

CC      = gcc
CFLAGS  = -Wall -Wextra -pedantic -pthread -g
COMMON  = ../common

CLI_SRCS = main.c client.c receiver.c input_handler.c \
           $(COMMON)/protocol.c $(COMMON)/utils.c
CLI_OBJS = $(CLI_SRCS:.c=.o)
CLI_BIN  = client

client: $(CLI_OBJS)
	$(CC) $(CFLAGS) -o $(CLI_BIN) $(CLI_OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(CLI_OBJS) $(CLI_BIN)

.PHONY: clean