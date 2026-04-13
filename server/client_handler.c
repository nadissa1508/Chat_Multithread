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
 * Archivo: server/client_handler.c
 * Descripción: Maneja la sesión completa de un cliente en su propio
 *              thread: espera el REGISTER inicial, luego despacha cada
 *              mensaje entrante (BROADCAST, MSG, LIST_USERS, USER_INFO,
 *              SET_STATUS, DISCONNECT, PONG) al handler correspondiente.
 *              Al terminar la sesión limpia el slot en user_manager y
 *              cierra el socket.
 */

#include "client_handler.h"
#include "user_manager.h"
#include "../common/protocol.h"
#include "../common/utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

// Internas

// Lee una línea completa del socket y retorna bytes leídos.
static int recv_line(int fd, char *buffer, int max_len) {
    int total = 0;
    char c;

    while (total < max_len - 1) {
        int n = recv(fd, &c, 1, 0);
        if (n <= 0) return n;   // 0 = cerrado, -1 = error 
        buffer[total++] = c;
        if (c == '\n') break;
    }
    buffer[total] = '\0';
    return total;
}

 // send_to_user
int send_to_user(const char *username, const char *msg_buffer) {
    int fd = um_get_socket(username);
    if (fd < 0) {
        log_warn("send_to_user: '%s' no encontrado", username);
        return -1;
    }
    int n = send(fd, msg_buffer, strlen(msg_buffer), 0);
    if (n < 0) log_warn("send_to_user: error enviando a '%s'", username);
    return n;
}

 // broadcast_message
void broadcast_message(const char *msg_buffer, const char *exclude) {
    /* Peor caso: 64 clientes * (50 username + 1 colon + 8 status + 1 coma)
     * = 64 * 60 = 3840 bytes. Mismo calculo que handle_list_users. */
    char list_buf[MAX_CLIENTS * (MAX_USERNAME + 8 + 2)];
    um_list_users(list_buf, sizeof(list_buf));

    char copy[sizeof(list_buf)];
    strncpy(copy, list_buf, sizeof(copy) - 1);
    copy[sizeof(copy) - 1] = '\0';

    char *saveptr = NULL;
    char *token = strtok_r(copy, ",", &saveptr);
    while (token != NULL) {
        // token = "username:STATUS" 
        char *colon = strchr(token, ':');
        if (colon) *colon = '\0';   //nos quedamos solo con el username 

        if (exclude == NULL || strcmp(token, exclude) != 0) {
            send_to_user(token, msg_buffer);
        }
        token = strtok_r(NULL, ",", &saveptr);
    }
}

 // Handlers de cada tipo de mensaje

// Envía un error genérico al cliente 
static void reply_error(int fd, const char *description) {
    char buf[MAX_MSG_LEN];
    make_message(MSG_ERROR, ORIGIN_SRV, "CLIENT", description, buf);
    send(fd, buf, strlen(buf), 0);
}

// REGISTER 
static int handle_register(int fd, const char *client_ip,
                            const Message *msg, char *username_out) {
    char buf[MAX_MSG_LEN];

    // El nombre de usuario va en ORIGIN 
    const char *username = msg->origin;

    if (!is_valid_username(username)) {
        make_message(MSG_REGISTER_ERR, ORIGIN_SRV, username,
                     "Nombre de usuario invalido", buf);
        send(fd, buf, strlen(buf), 0);
        return -1;
    }

    int res = um_add_user(username, client_ip, fd, pthread_self());
    switch (res) {
        case 0:
            strncpy(username_out, username, MAX_USERNAME - 1);
            make_message(MSG_REGISTER_OK, ORIGIN_SRV, username,
                         "Bienvenido al chat", buf);
            send(fd, buf, strlen(buf), 0);
            log_info("REGISTER OK: '%s' desde %s", username, client_ip);
            return 0;
        case -1:
            make_message(MSG_REGISTER_ERR, ORIGIN_SRV, username,
                         "Usuario ya registrado", buf);
            break;
        case -2:
            make_message(MSG_REGISTER_ERR, ORIGIN_SRV, username,
                         "Servidor lleno", buf);
            break;
        default:
            make_message(MSG_REGISTER_ERR, ORIGIN_SRV, username,
                         "Error desconocido", buf);
    }
    send(fd, buf, strlen(buf), 0);
    return -1;
}

// BROADCAST 
static void handle_broadcast(const Message *msg) {
    char buf[MAX_MSG_LEN];
    make_message(MSG_DELIVER, msg->origin, DEST_ALL, msg->content, buf);
    broadcast_message(buf, msg->origin);  // excluir al emisor 
}

// MSG (mensaje directo) 
static void handle_direct(int fd, const Message *msg) {
    char buf[MAX_MSG_LEN];
    const char *dest = msg->destination;

    if (!um_exists(dest)) {
        make_message(MSG_ERROR, ORIGIN_SRV, msg->origin,
                     "Usuario destino no conectado", buf);
        send(fd, buf, strlen(buf), 0);
        return;
    }

    make_message(MSG_DELIVER, msg->origin, dest, msg->content, buf);

    /* send_to_user puede fallar si el destinatario se desconecto entre
     * um_exists y este punto. En ese caso se notifica al emisor para 
     *que no asuma que el mensaje fue entregado. */
    if (send_to_user(dest, buf) < 0) {
        char err[MAX_MSG_LEN];
        make_message(MSG_ERROR, ORIGIN_SRV, msg->origin,
                     "El usuario se desconecto antes de recibir el mensaje",
                     err);
        send(fd, err, strlen(err), 0);
    }
}

// LIST_USERS
static void handle_list_users(int fd, const Message *msg) {
    char list[MAX_CLIENTS * (MAX_USERNAME + 8 + 2)];
    char buf[MAX_MSG_LEN];

    um_list_users(list, sizeof(list));
    make_message(MSG_LIST_USERS_OK, ORIGIN_SRV, msg->origin, list, buf);
    send(fd, buf, strlen(buf), 0);
}

// USER_INFO
static void handle_user_info(int fd, const Message *msg) {
    char buf[MAX_MSG_LEN];
    char ip[INET_ADDRSTRLEN];
    const char *target = msg->content;

    if (um_get_ip(target, ip) == 0) {
        // Formato: "username:IP" 
        char info[MAX_USERNAME + INET_ADDRSTRLEN + 2];
        snprintf(info, sizeof(info), "%.49s:%s", target, ip);
        make_message(MSG_USER_INFO_OK, ORIGIN_SRV, msg->origin, info, buf);
    } else {
        make_message(MSG_USER_INFO_ERR, ORIGIN_SRV, msg->origin,
                     "Usuario no encontrado", buf);
    }
    send(fd, buf, strlen(buf), 0);
}

// SET_STATUS
static void handle_set_status(int fd, const Message *msg) {
    char buf[MAX_MSG_LEN];
    const char *new_status = msg->content;

    int res = um_set_status(msg->origin, new_status);
    if (res == 0) {
        make_message(MSG_STATUS_OK, ORIGIN_SRV, msg->origin, new_status, buf);
    } else {
        make_message(MSG_ERROR, ORIGIN_SRV, msg->origin,
                     "Status invalido o usuario no encontrado", buf);
    }
    send(fd, buf, strlen(buf), 0);
}

// DISCONNECT
static void handle_disconnect(int fd, const char *username) {
    char buf[MAX_MSG_LEN];
    make_message(MSG_DISCONNECT_OK, ORIGIN_SRV, username,
                 "Sesion cerrada correctamente", buf);
    send(fd, buf, strlen(buf), 0);
}

// PONG — el cliente respondio al PING
static void handle_pong(const char *username) {
    um_reset_ping(username);
    log_debug("PONG recibido de '%s'", username);
}

 // handle_client — función principal del thread
void *handle_client(void *arg) {
    ClientArgs *ca = (ClientArgs *)arg;
    int    fd      = ca->socket_fd;
    char   client_ip[INET_ADDRSTRLEN];

    inet_ntop(AF_INET, &ca->client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
    free(ca);   // ya no necesitamos el argumento 

    log_info("Nueva conexión desde %s (fd=%d)", client_ip, fd);

    char     raw[MAX_MSG_LEN];
    Message  msg;
    char     username[MAX_USERNAME] = {0};   // vacío hasta REGISTER exitoso
    int      registered = 0;

    // Fase 1: esperar REGISTER 
    while (!registered) {
        int n = recv_line(fd, raw, sizeof(raw));
        if (n <= 0) {
            log_info("Conexión cerrada antes de REGISTER (fd=%d)", fd);
            close(fd);
            return NULL;
        }

        if (parse_message(raw, &msg) < 0) {
            reply_error(fd, "Mensaje mal formado");
            continue;
        }

        if (strcmp(msg.type, MSG_REGISTER) != 0) {
            reply_error(fd, "Debe registrarse primero con REGISTER");
            continue;
        }

        if (handle_register(fd, client_ip, &msg, username) == 0) {
            registered = 1;
        } else {
            // Registro rechazado: cerramos la conexión 
            close(fd);
            return NULL;
        }
    }

    // Fase 2: loop principal de mensajes 
    while (1) {
        int n = recv_line(fd, raw, sizeof(raw));

        if (n == 0) {
            // Cierre abrupto: el cliente cerró el socket sin DISCONNECT
            log_warn("Cierre abrupto de '%s' (fd=%d)", username, fd);
            break;
        }
        if (n < 0) {
            log_warn("Error de recv para '%s'", username);
            break;
        }

        if (parse_message(raw, &msg) < 0) {
            reply_error(fd, "Mensaje mal formado");
            continue;
        }

        /* Actualizar actividad. Si el usuario estaba INACTIVO y acaba de
         * enviar cualquier mensaje, el servidor lo restaura al estado previo
         * (ACTIVO u OCUPADO segun lo que tenia antes) y notifica al cliente. */
        int restored = um_update_activity(username);
        if (restored) {
            char restored_status[MAX_FIELD_LEN];
            um_get_status(username, restored_status);
            char status_buf[MAX_MSG_LEN];
            make_message(MSG_STATUS_UPDATE, ORIGIN_SRV, username,
                         restored_status, status_buf);
             // Enviar SOLO al usuario afectado (no broadcast a todos) 
            send(fd, status_buf, strlen(status_buf), 0);
        }

        // Despachar por tipo 
        if (strcmp(msg.type, MSG_BROADCAST) == 0) {
            handle_broadcast(&msg);

        } else if (strcmp(msg.type, MSG_DIRECT) == 0) {
            handle_direct(fd, &msg);

        } else if (strcmp(msg.type, MSG_LIST_USERS) == 0) {
            handle_list_users(fd, &msg);

        } else if (strcmp(msg.type, MSG_USER_INFO) == 0) {
            handle_user_info(fd, &msg);

        } else if (strcmp(msg.type, MSG_SET_STATUS) == 0) {
            handle_set_status(fd, &msg);

        } else if (strcmp(msg.type, MSG_DISCONNECT) == 0) {
            handle_disconnect(fd, username);
            break;   // salir del loop limpiamente 

        } else if (strcmp(msg.type, MSG_PONG) == 0) {
            handle_pong(username);

        } else {
            log_warn("Tipo desconocido '%s' de '%s'", msg.type, username);
            reply_error(fd, "Tipo de mensaje desconocido");
        }
    }

    // Limpieza 
    if (username[0] != '\0') {
        um_remove_user(username);
    }
    close(fd);
    log_info("Sesión de '%s' finalizada", username);
    return NULL;
}