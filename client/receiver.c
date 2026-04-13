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
 * Archivo: client/receiver.c
 * Descripción: Thread receptor del cliente. Lee mensajes del servidor
 *              byte a byte, los parsea y los presenta con formato visual
 *              según su tipo (MSG_DELIVER, LIST_USERS_OK, STATUS_UPDATE,
 *              PING, DISCONNECT_OK, ERROR). Responde automáticamente a
 *              PING con PONG sin interrumpir al usuario.
 */

#include "receiver.h"
#include "client.h"
#include "../common/protocol.h"
#include "../common/utils.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>

 // Helpers de presentación

// Imprime un mensaje recibido con formato visual claro
static void print_incoming(const Message *msg) {
    if (strcmp(msg->destination, DEST_ALL) == 0) {
        // Broadcast
        printf("\n[TODOS] %s: %s\n> ", msg->origin, msg->content);
    } else {
        // Mensaje directo
        printf("\n[PRIVADO] %s → %s: %s\n> ", 
               msg->origin, msg->destination, msg->content);
    }
    fflush(stdout);
}

static void print_list(const Message *msg) {
    printf("\n=== Usuarios conectados ===\n");
    // Formato: "user1:STATUS,user2:STATUS,..."
    char copy[MAX_CONTENT_LEN];
    strncpy(copy, msg->content, MAX_CONTENT_LEN - 1);

    char *saveptr = NULL;
    char *token = strtok_r(copy, ",", &saveptr);
    while (token != NULL) {
        // Reemplazar ':' por ' — ' para mejor lectura 
        char *colon = strchr(token, ':');
        if (colon) {
            *colon = '\0';
            printf("  • %-20s %s\n", token, colon + 1);
        } else {
            printf("  • %s\n", token);
        }
        token = strtok_r(NULL, ",", &saveptr);
    }
    printf("===========================\n> ");
    fflush(stdout);
}

static void print_user_info(const Message *msg) {
    // Formato del contenido: "username:IP" 
    printf("\n[INFO] %s\n> ", msg->content);
    fflush(stdout);
}

static void print_status_update(const Message *msg) {
    strncpy(g_state.status, msg->content, MAX_FIELD_LEN - 1);
    g_state.status[MAX_FIELD_LEN - 1] = '\0';

    /* El servidor envia STATUS_UPDATE tanto para marcar INACTIVO por
     * inactividad como para restaurar a ACTIVO cuando el usuario retoma
     * actividad. El mensaje que se muestra debe reflejar cual caso es. */
    if (strcmp(msg->content, STATUS_INACTIVE) == 0) {
        printf("\n[STATUS] Tu estado fue cambiado a %s por inactividad.\n> ",
               msg->content);
    } else {
        printf("\n[STATUS] Tu estado fue restaurado a %s.\n> ",
               msg->content);
    }
    fflush(stdout);
}

static void print_error(const Message *msg) {
    printf("\n[ERROR] %s\n> ", msg->content);
    fflush(stdout);
}

 // receive_messages — función del thread
void *receive_messages(void *arg) {
    (void)arg;

    char    raw[MAX_MSG_LEN];
    Message msg;
    int     fd = g_state.socket_fd;

    while (g_state.running) {
        // Leer byte a byte hasta '\n' — igual que en el servidor
        int total = 0;
        char c;
        int ok = 1;

        while (total < (int)sizeof(raw) - 1) {
            int n = recv(fd, &c, 1, 0);
            if (n <= 0) {
                ok = 0;
                break;
            }
            raw[total++] = c;
            if (c == '\n') break;
        }
        raw[total] = '\0';

        if (!ok) {
            printf("\n[SISTEMA] Conexion con el servidor perdida.\n");
            printf("[SISTEMA] Presiona Enter para salir.\n");
            fflush(stdout);
            g_state.running = 0;
            /* Cancelar el thread de input para que no quede bloqueado en
             * fgets indefinidamente. pthread_cancel interrumpe fgets porque
             * es un punto de cancelacion POSIX. El join en main() limpiara. */
            if (g_state.input_thread != 0)
                pthread_cancel(g_state.input_thread);
            break;
        }

        if (parse_message(raw, &msg) < 0) {
            log_warn("receiver: mensaje mal formado ignorado");
            continue;
        }

        // Despachar por tipo 
        if (strcmp(msg.type, MSG_DELIVER) == 0) {
            print_incoming(&msg);

        } else if (strcmp(msg.type, MSG_LIST_USERS_OK) == 0) {
            print_list(&msg);

        } else if (strcmp(msg.type, MSG_USER_INFO_OK) == 0 ||
                   strcmp(msg.type, MSG_USER_INFO_ERR) == 0) {
            print_user_info(&msg);

        } else if (strcmp(msg.type, MSG_STATUS_OK) == 0) {
            strncpy(g_state.status, msg.content, MAX_FIELD_LEN - 1);
            printf("\n[STATUS] Estado cambiado a %s\n> ", msg.content);
            fflush(stdout);

        } else if (strcmp(msg.type, MSG_STATUS_UPDATE) == 0) {
            print_status_update(&msg);

        } else if (strcmp(msg.type, MSG_PING) == 0) {
            // Responder PONG automáticamente sin interrumpir al usuario 
            char buf[MAX_MSG_LEN];
            make_message(MSG_PONG, g_state.username, ORIGIN_SRV, "", buf);
            send(fd, buf, strlen(buf), 0);

        } else if (strcmp(msg.type, MSG_DISCONNECT_OK) == 0) {
            printf("\n[SISTEMA] Desconectado correctamente.\n");
            g_state.running = 0;
            break;

        } else if (strcmp(msg.type, MSG_ERROR) == 0) {
            print_error(&msg);

        } else {
            log_debug("receiver: tipo '%s' no manejado", msg.type);
        }
    }

    return NULL;
}