/*
 * Universidad del Valle de Guatemala
 * Departamento de Computación
 * Sistemas Operativos - CC3064
 *
 * Autores:
 *  - Angie Vela, 23764
 *  - Adrián González, 23152
 *  - Paula De León, 23202
 *
 * Archivo: client/main.c
 * Descripción: Punto de entrada del cliente de chat. Valida argumentos,
 *              establece la conexión TCP, realiza el REGISTER y lanza
 *              dos threads concurrentes: el receptor de mensajes y el
 *              manejador de input del usuario.
 *              Uso: ./client <usuario> <IP_servidor> <puerto>
 */

#include "client.h"
#include "receiver.h"
#include "input_handler.h"
#include "../common/utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <usuario> <IP_servidor> <puerto>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *username  = argv[1];
    const char *server_ip = argv[2];
    int         port      = atoi(argv[3]);

    if (!is_valid_username(username)) {
        fprintf(stderr, "Nombre de usuario inválido: '%s'\n", username);
        fprintf(stderr, "  → Sin espacios, '|' ni caracteres especiales. Máx %d chars.\n",
                MAX_USERNAME);
        return EXIT_FAILURE;
    }

    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Puerto inválido: %s\n", argv[3]);
        return EXIT_FAILURE;
    }

    // Ignorar SIGPIPE: si el servidor cierra la conexion mientras el cliente
    // intenta enviar, send() retornara -1 en lugar de terminar el proceso.
    signal(SIGPIPE, SIG_IGN);

    log_set_level(LOG_WARN);

    // 1. Conectar 
    int fd = client_connect(server_ip, port);
    if (fd < 0) return EXIT_FAILURE;

    // 2. Registrar 
    if (client_register(fd, username) < 0) {
        close(fd);
        return EXIT_FAILURE;
    }

    // Inicializar estado global 
    g_state.socket_fd = fd;
    g_state.running   = 1;
    strncpy(g_state.username, username, MAX_USERNAME - 1);
    strncpy(g_state.status,   STATUS_ACTIVE, MAX_FIELD_LEN - 1);

    printf("\n¡Bienvenido al chat, %s! Escribe /help para ver los comandos.\n",
           username);

    // 3. Thread receptor
    pthread_t recv_thread;
    if (pthread_create(&recv_thread, NULL, receive_messages, NULL) != 0) {
        fprintf(stderr, "Error creando thread receptor\n");
        close(fd);
        return EXIT_FAILURE;
    }

    /* 4. Thread de input (en el thread principal)
     * Guardar el ID del thread principal en g_state para que el receiver
     * pueda cancelarlo con pthread_cancel si el servidor muere. */
    g_state.input_thread = pthread_self();

    /* Habilitar cancelacion diferida: pthread_cancel interrumpira en el
     * primer punto de cancelacion POSIX que encuentre (fgets lo es). */
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

    input_loop(NULL);

    // 5. Esperar al receptor y limpiar
    g_state.running = 0;
    pthread_join(recv_thread, NULL);
    close(fd);

    printf("Hasta luego, %s.\n", username);
    return EXIT_SUCCESS;
}