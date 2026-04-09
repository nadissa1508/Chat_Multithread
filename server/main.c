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
 * Archivo: server/main.c
 * Descripción: Punto de entrada del servidor de chat. Valida el puerto
 *              recibido por argumento, configura la señal SIGPIPE y el
 *              nivel de log, luego delega el arranque completo a start_server().
 */

#include "server.h"
#include "../common/utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <puerto>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int port = atoi(argv[1]);
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Puerto inválido: %s\n", argv[1]);
        return EXIT_FAILURE;
    }

    /* Ignorar SIGPIPE: si send() escribe en un socket cerrado el sistema
     * generaria esta senal terminando el proceso. Con SIG_IGN, send()
     * retorna -1 con errno=EPIPE y el codigo puede manejar el error. */
    signal(SIGPIPE, SIG_IGN);

    /* Nivel de log: cambiar a LOG_DEBUG para más detalle */
    log_set_level(LOG_INFO);

    log_info("=== Servidor de Chat iniciando en puerto %d ===", port);

    if (start_server(port) < 0) {
        log_error("No se pudo iniciar el servidor");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}