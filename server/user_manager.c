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
 * Archivo: server/user_manager.c
 * Descripción: Implementación thread-safe de la tabla de usuarios conectados.
 *              Provee operaciones de registro, eliminación y consulta de usuarios,
 *              control de estado (ACTIVO, OCUPADO, INACTIVO) y detección de
 *              inactividad mediante mecanismo de PING/PONG, todo protegido
 *              por mutex para evitar condiciones de carrera.
 */

#include "user_manager.h"
#include "../common/utils.h"
#include "../common/protocol.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

/* =========================================================
 * Estado global — protegido por users_mutex
 * ========================================================= */
static UserEntry  users[MAX_CLIENTS];
static int        user_count = 0;
static pthread_mutex_t users_mutex = PTHREAD_MUTEX_INITIALIZER;

/* =========================================================
 * um_init
 * ========================================================= */
void um_init(void) {
    pthread_mutex_lock(&users_mutex);
    memset(users, 0, sizeof(users));
    user_count = 0;
    pthread_mutex_unlock(&users_mutex);
    log_info("UserManager inicializado (máx. %d clientes)", MAX_CLIENTS);
}

/* =========================================================
 * um_add_user
 * ========================================================= */
int um_add_user(const char *username,
                const char *ip,
                int         socket_fd,
                pthread_t   thread_id) {

    pthread_mutex_lock(&users_mutex);   /* ← INICIO SECCIÓN CRÍTICA */

    /* Verificar duplicados DENTRO del mutex (evita race condition Caso 1) */
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!users[i].active) continue;

        if (strcmp(users[i].username, username) == 0) {
            pthread_mutex_unlock(&users_mutex);
            log_warn("um_add_user: username '%s' ya existe", username);
            return -1;  /* username duplicado */
        }
        /* Nota: la validacion de IP duplicada se omite intencionalmente
         * para permitir multiples clientes desde la misma maquina durante
         * el desarrollo y las pruebas. En produccion se activaria aqui. */
    }

    /* Buscar slot libre e insertar */
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!users[i].active) {
            users[i].active      = 1;
            users[i].socket_fd   = socket_fd;
            users[i].thread_id   = thread_id;
            users[i].last_activity = time(NULL);

            strncpy(users[i].username, username, MAX_USERNAME - 1);
            strncpy(users[i].ip,       ip,       INET_ADDRSTRLEN - 1);
            strncpy(users[i].status,      STATUS_ACTIVE, MAX_FIELD_LEN - 1);
            strncpy(users[i].prev_status, STATUS_ACTIVE, MAX_FIELD_LEN - 1);

            user_count++;
            pthread_mutex_unlock(&users_mutex);     /* ← FIN SECCIÓN CRÍTICA */

            log_info("Usuario registrado: '%s' desde %s (fd=%d)", username, ip, socket_fd);
            return 0;
        }
    }

    pthread_mutex_unlock(&users_mutex);
    log_warn("um_add_user: tabla llena (%d clientes)", MAX_CLIENTS);
    return -2;  /* tabla llena */
}

/* =========================================================
 * um_remove_user
 * ========================================================= */
int um_remove_user(const char *username) {
    pthread_mutex_lock(&users_mutex);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (users[i].active && strcmp(users[i].username, username) == 0) {
            users[i].active    = 0;
            users[i].socket_fd = -1;
            memset(users[i].username, 0, MAX_USERNAME);
            user_count--;

            pthread_mutex_unlock(&users_mutex);
            log_info("Usuario eliminado: '%s'", username);
            return 0;
        }
    }

    pthread_mutex_unlock(&users_mutex);
    log_warn("um_remove_user: '%s' no encontrado", username);
    return -1;
}

/* =========================================================
 * um_exists
 * ========================================================= */
int um_exists(const char *username) {
    pthread_mutex_lock(&users_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (users[i].active && strcmp(users[i].username, username) == 0) {
            pthread_mutex_unlock(&users_mutex);
            return 1;
        }
    }
    pthread_mutex_unlock(&users_mutex);
    return 0;
}

/* =========================================================
 * um_get_socket
 * Protegido por mutex para evitar race condition Caso 2:
 * no se puede leer el fd mientras otro thread lo invalida.
 * ========================================================= */
int um_get_socket(const char *username) {
    pthread_mutex_lock(&users_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (users[i].active && strcmp(users[i].username, username) == 0) {
            int fd = users[i].socket_fd;
            pthread_mutex_unlock(&users_mutex);
            return fd;
        }
    }
    pthread_mutex_unlock(&users_mutex);
    return -1;
}

/* =========================================================
 * um_get_ip
 * ========================================================= */
int um_get_ip(const char *username, char *ip_out) {
    pthread_mutex_lock(&users_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (users[i].active && strcmp(users[i].username, username) == 0) {
            strncpy(ip_out, users[i].ip, INET_ADDRSTRLEN - 1);
            ip_out[INET_ADDRSTRLEN - 1] = '\0';
            pthread_mutex_unlock(&users_mutex);
            return 0;
        }
    }
    pthread_mutex_unlock(&users_mutex);
    return -1;
}

/* =========================================================
 * um_get_status
 * ========================================================= */
int um_get_status(const char *username, char *status_out) {
    pthread_mutex_lock(&users_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (users[i].active && strcmp(users[i].username, username) == 0) {
            strncpy(status_out, users[i].status, MAX_FIELD_LEN - 1);
            status_out[MAX_FIELD_LEN - 1] = '\0';
            pthread_mutex_unlock(&users_mutex);
            return 0;
        }
    }
    pthread_mutex_unlock(&users_mutex);
    return -1;
}

/* =========================================================
 * um_list_users
 * ========================================================= */
int um_list_users(char *buffer, size_t buf_size) {
    pthread_mutex_lock(&users_mutex);

    int count   = 0;
    size_t pos  = 0;
    buffer[0]   = '\0';

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!users[i].active) continue;

        /* Formato: "username:STATUS," */
        int written = snprintf(buffer + pos, buf_size - pos,
                               "%s:%s,",
                               users[i].username,
                               users[i].status);
        if (written < 0 || (size_t)written >= buf_size - pos) break;

        pos += written;
        count++;
    }

    /* Eliminar la coma final */
    if (pos > 0 && buffer[pos - 1] == ',') {
        buffer[pos - 1] = '\0';
    }

    pthread_mutex_unlock(&users_mutex);
    return count;
}

/* =========================================================
 * um_set_status
 * ========================================================= */
int um_set_status(const char *username, const char *new_status) {
    /* Validar que el status sea uno de los permitidos */
    if (strcmp(new_status, STATUS_ACTIVE)   != 0 &&
        strcmp(new_status, STATUS_BUSY)     != 0 &&
        strcmp(new_status, STATUS_INACTIVE) != 0) {
        log_warn("um_set_status: status inválido '%s'", new_status);
        return -2;
    }

    pthread_mutex_lock(&users_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (users[i].active && strcmp(users[i].username, username) == 0) {
            strncpy(users[i].status, new_status, MAX_FIELD_LEN - 1);
            /* Guardar como estado previo para restauracion post-inactividad,
             * excepto si el nuevo estado es INACTIVO (ese solo lo asigna
             * el servidor automaticamente, nunca debe ser el prev). */
            if (strcmp(new_status, STATUS_INACTIVE) != 0)
                strncpy(users[i].prev_status, new_status, MAX_FIELD_LEN - 1);
            pthread_mutex_unlock(&users_mutex);
            log_info("Status de '%s' -> %s", username, new_status);
            return 0;
        }
    }
    pthread_mutex_unlock(&users_mutex);
    return -1;
}

/* =========================================================
 * um_update_activity
 * ========================================================= */
int um_update_activity(const char *username) {
    int restored = 0;
    pthread_mutex_lock(&users_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (users[i].active && strcmp(users[i].username, username) == 0) {
            users[i].last_activity = time(NULL);
            users[i].ping_sent     = 0;
            if (strcmp(users[i].status, STATUS_INACTIVE) == 0) {
                /* Restaurar al estado que tenia antes de ser marcado INACTIVO.
                 * Si estaba ACTIVO, vuelve a ACTIVO. Si estaba OCUPADO, vuelve
                 * a OCUPADO. Si prev_status esta vacio por alguna razon,
                 * se usa ACTIVO como fallback. */
                const char *restore = (users[i].prev_status[0] != '\0')
                                      ? users[i].prev_status
                                      : STATUS_ACTIVE;
                strncpy(users[i].status, restore, MAX_FIELD_LEN - 1);
                restored = 1;
                log_info("'%s' restaurado a %s por actividad", username, restore);
            }
            break;
        }
    }
    pthread_mutex_unlock(&users_mutex);
    return restored;
}

/* =========================================================
 * um_check_inactivity
 * ========================================================= */
int um_check_inactivity(void) {
    time_t now      = time(NULL);
    int    marked   = 0;

    pthread_mutex_lock(&users_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!users[i].active) continue;
        if (strcmp(users[i].status, STATUS_ACTIVE) != 0) continue;

        double elapsed = difftime(now, users[i].last_activity);
        if (elapsed >= INACTIVITY_SECS) {
            strncpy(users[i].prev_status, users[i].status, MAX_FIELD_LEN - 1);
            strncpy(users[i].status, STATUS_INACTIVE, MAX_FIELD_LEN - 1);
            log_info("'%s' marcado INACTIVO (%.0fs sin actividad)", 
                     users[i].username, elapsed);
            marked++;
            /* Nota: el thread del cliente enviará STATUS_UPDATE al usuario.
             * No lo hacemos aquí para no tener I/O dentro del mutex. */
        }
    }
    pthread_mutex_unlock(&users_mutex);

    return marked;
}

/* =========================================================
 * um_reset_ping — llego PONG del cliente
 * ========================================================= */
void um_reset_ping(const char *username) {
    pthread_mutex_lock(&users_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (users[i].active && strcmp(users[i].username, username) == 0) {
            users[i].ping_sent     = 0;
            users[i].last_activity = time(NULL);
            /* Si el usuario estaba INACTIVO, restaurar al estado previo */
            if (strcmp(users[i].status, STATUS_INACTIVE) == 0) {
                const char *restore = (users[i].prev_status[0] != '\0')
                                      ? users[i].prev_status
                                      : STATUS_ACTIVE;
                strncpy(users[i].status, restore, MAX_FIELD_LEN - 1);
            }
            break;
        }
    }
    pthread_mutex_unlock(&users_mutex);
    log_debug("um_reset_ping: PONG de '%s'", username);
}

/* =========================================================
 * um_collect_inactivity — revision completa bajo un mutex
 * ========================================================= */
void um_collect_inactivity(char needs_ping[][MAX_USERNAME],
                           int  ping_socks[],
                           int *n_ping,
                           char needs_inactive[][MAX_USERNAME],
                           int  inactive_socks[],
                           int *n_inactive,
                           int  ping_wait_secs) {
    *n_ping     = 0;
    *n_inactive = 0;
    time_t now  = time(NULL);

    pthread_mutex_lock(&users_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!users[i].active) continue;
        if (strcmp(users[i].status, STATUS_INACTIVE) == 0) continue;

        if (!users[i].ping_sent) {
            double idle = difftime(now, users[i].last_activity);
            if (idle >= INACTIVITY_SECS) {
                users[i].ping_sent = 1;
                users[i].ping_time = now;
                strncpy(needs_ping[*n_ping], users[i].username, MAX_USERNAME - 1);
                needs_ping[*n_ping][MAX_USERNAME - 1] = '\0';
                ping_socks[*n_ping] = users[i].socket_fd;
                (*n_ping)++;
            }
        } else {
            double waited = difftime(now, users[i].ping_time);
            if (waited >= ping_wait_secs) {
                strncpy(users[i].prev_status, users[i].status, MAX_FIELD_LEN - 1);
                strncpy(users[i].status, STATUS_INACTIVE, MAX_FIELD_LEN - 1);
                users[i].ping_sent = 0;
                strncpy(needs_inactive[*n_inactive], users[i].username, MAX_USERNAME - 1);
                needs_inactive[*n_inactive][MAX_USERNAME - 1] = '\0';
                inactive_socks[*n_inactive] = users[i].socket_fd;
                (*n_inactive)++;
                log_info("'%s' marcado INACTIVO (sin PONG tras %.0fs)",
                         users[i].username, waited);
            }
        }
    }
    pthread_mutex_unlock(&users_mutex);
}