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
 * Archivo: client/client.c
 * Descripción: Implementa la conexión TCP al servidor y la fase de
 *              registro (handshake REGISTER / REGISTER_OK). Define e
 *              inicializa el estado global g_state compartido entre
 *              los threads de input y recepción.
 */

#include "client.h"
#include "../common/protocol.h"
#include "../common/utils.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

// Instancia global del estado 
ClientState g_state = {
    .socket_fd    = -1,
    .username     = {0},
    .status       = STATUS_ACTIVE,
    .running      = 0,
    .input_thread = 0
};


// client_connect
int client_connect(const char *server_ip, int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        log_error("socket(): %s", strerror(errno));
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);

    if (inet_pton(AF_INET, server_ip, &addr.sin_addr) <= 0) {
        log_error("IP inválida: %s", server_ip);
        close(fd);
        return -1;
    }

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        log_error("connect() a %s:%d — %s", server_ip, port, strerror(errno));
        close(fd);
        return -1;
    }

    log_info("Conectado al servidor %s:%d", server_ip, port);
    return fd;
}

// client_register
int client_register(int fd, const char *username) {
    char buf[MAX_MSG_LEN];
    char raw[MAX_MSG_LEN];

    // Enviar REGISTER — ORIGIN es el nombre de usuario 
    make_message(MSG_REGISTER, username, ORIGIN_SRV, "", buf);
    if (send(fd, buf, strlen(buf), 0) < 0) {
        log_error("send(REGISTER): %s", strerror(errno));
        return -1;
    }

    // Leer la respuesta byte a byte hasta '\n' para no consumir
    // bytes de mensajes posteriores (broadcast de bienvenida, etc.)
    int total = 0;
    char c;
    while (total < (int)sizeof(raw) - 1) {
        int n = recv(fd, &c, 1, 0);
        if (n <= 0) {
            log_error("Sin respuesta del servidor al registrar");
            return -1;
        }
        raw[total++] = c;
        if (c == '\n') break;
    }
    raw[total] = '\0';

    Message msg;
    if (parse_message(raw, &msg) < 0) {
        log_error("Respuesta de registro mal formada");
        return -1;
    }

    if (strcmp(msg.type, MSG_REGISTER_OK) == 0) {
        log_info("Registro exitoso: %s", msg.content);
        return 0;
    } else {
        fprintf(stderr, "Error de registro: %s\n", msg.content);
        return -1;
    }
}