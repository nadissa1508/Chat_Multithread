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
 * Archivo: server/server.c
 * Descripción: Núcleo del servidor TCP. Inicializa el socket, realiza
 *              bind y listen, lanza un thread de inactividad que revisa
 *              periódicamente el estado de los clientes, y entra al
 *              accept loop creando un thread dedicado por cada conexión.
 */

#include "server.h"
#include "client_handler.h"
#include "user_manager.h"
#include "../common/utils.h"
#include "../common/protocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

/* Intervalo en segundos entre cada revisión de inactividad */
#define INACTIVITY_CHECK_INTERVAL 10

/* =========================================================
 * Thread de inactividad
 * Corre en paralelo al accept loop.
 * Cada INACTIVITY_CHECK_INTERVAL segundos revisa la tabla de
 * usuarios y marca como INACTIVO a quien lleve >= INACTIVITY_SECS
 * sin actividad. Luego notifica a cada afectado vía STATUS_UPDATE.
 * ========================================================= */
static void *inactivity_thread(void *arg) {
    (void)arg;  /* no usamos el argumento */

    log_info("Thread de inactividad iniciado (chequeo cada %ds)", 
             INACTIVITY_CHECK_INTERVAL);

    while (1) {
        sleep(INACTIVITY_CHECK_INTERVAL);

        /* um_check_inactivity marca en la tabla pero NO envía mensajes
         * (no hace I/O dentro del mutex). Aquí obtenemos la lista de
         * inactivos y les enviamos STATUS_UPDATE fuera del mutex. */

        /* Obtener lista completa y filtrar los INACTIVOS */
        char list_buf[MAX_MSG_LEN * 2];
        um_list_users(list_buf, sizeof(list_buf));

        /* Primero ejecutar el chequeo (marca en la tabla) */
        int marked = um_check_inactivity();

        if (marked > 0) {
            /* Recorrer la lista y notificar a los ahora INACTIVOS */
            char copy[sizeof(list_buf)];
            strncpy(copy, list_buf, sizeof(copy) - 1);

            /* Necesitamos la lista ACTUALIZADA (post-marcado) */
            um_list_users(copy, sizeof(copy));

            char *saveptr = NULL;
            char *token = strtok_r(copy, ",", &saveptr);
            while (token != NULL) {
                char *colon = strchr(token, ':');
                if (colon && strcmp(colon + 1, STATUS_INACTIVE) == 0) {
                    *colon = '\0';  /* token ahora es solo el username */
                    char buf[MAX_MSG_LEN];
                    make_message(MSG_STATUS_UPDATE, ORIGIN_SRV, token,
                                 STATUS_INACTIVE, buf);
                    /* Enviar SOLO al usuario afectado (no broadcast a todos) */
                    send_to_user(token, buf);
                }
                token = strtok_r(NULL, ",", &saveptr);
            }
        }
    }
    return NULL;
}

/* =========================================================
 * start_server
 * ========================================================= */
int start_server(int port) {
    /* ── 1. Inicializar user_manager ─────────────────────── */
    um_init();

    /* ── 2. Crear socket TCP ──────────────────────────────── */
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        log_error("socket(): %s", strerror(errno));
        return -1;
    }

    /* SO_REUSEADDR: permite reusar el puerto inmediatamente tras reiniciar */
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        log_warn("setsockopt(SO_REUSEADDR): %s", strerror(errno));
    }

    /* ── 3. Bind ──────────────────────────────────────────── */
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;      /* escucha en todas las interfaces */
    addr.sin_port        = htons(port);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        log_error("bind() en puerto %d: %s", port, strerror(errno));
        close(server_fd);
        return -1;
    }

    /* ── 4. Listen ────────────────────────────────────────── */
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        log_error("listen(): %s", strerror(errno));
        close(server_fd);
        return -1;
    }

    log_info("Servidor escuchando en puerto %d (máx. %d clientes)", 
             port, MAX_CLIENTS);

    /* ── 5. Thread de inactividad ─────────────────────────── */
    pthread_t inact_thread;
    if (pthread_create(&inact_thread, NULL, inactivity_thread, NULL) != 0) {
        log_error("pthread_create (inactividad): %s", strerror(errno));
        close(server_fd);
        return -1;
    }
    pthread_detach(inact_thread);   /* no necesitamos join */

    /* ── 6. Accept loop ───────────────────────────────────── */
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(server_fd,
                               (struct sockaddr *)&client_addr,
                               &client_len);
        if (client_fd < 0) {
            log_warn("accept(): %s", strerror(errno));
            continue;   /* seguir esperando conexiones */
        }

        /* Preparar argumento para el thread (heap, no stack) */
        ClientArgs *ca = malloc(sizeof(ClientArgs));
        if (ca == NULL) {
            log_error("malloc ClientArgs: sin memoria");
            close(client_fd);
            continue;
        }
        ca->socket_fd   = client_fd;
        ca->client_addr = client_addr;

        /* Crear thread por cliente */
        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, ca) != 0) {
            log_error("pthread_create (cliente): %s", strerror(errno));
            free(ca);
            close(client_fd);
            continue;
        }
        pthread_detach(tid);    /* el thread se limpia solo al terminar */
    }

    close(server_fd);
    return 0;
}