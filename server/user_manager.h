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
 * Archivo: server/user_manager.h
 * Descripción: Interfaz pública del gestor de usuarios. Define la estructura
 *              UserEntry con los campos de cada cliente conectado (socket,
 *              username, IP, estado, actividad, ping) y declara toda la API
 *              para administrar la tabla de usuarios de forma thread-safe.
 */

#ifndef USER_MANAGER_H
#define USER_MANAGER_H

#include <pthread.h>
#include <netinet/in.h>
#include <time.h>
#include "../common/protocol.h"

#define MAX_CLIENTS     64
#define INACTIVITY_SECS 30

typedef struct {
    int    active;
    int    socket_fd;
    char   username[MAX_USERNAME];
    char   ip[INET_ADDRSTRLEN];
    char   status[MAX_FIELD_LEN];
    time_t last_activity;
    pthread_t thread_id;
    int    ping_sent;
    time_t ping_time;
    char   prev_status[MAX_FIELD_LEN]; /* estado antes de ser marcado INACTIVO */
} UserEntry;

void um_init(void);

int  um_add_user(const char *username, const char *ip,
                 int socket_fd, pthread_t thread_id);
int  um_remove_user(const char *username);

int  um_exists(const char *username);
int  um_get_socket(const char *username);
int  um_get_ip(const char *username, char *ip_out);
int  um_get_status(const char *username, char *status_out);
int  um_list_users(char *buffer, size_t buf_size);

int  um_set_status(const char *username, const char *new_status);

/*
 * um_update_activity
 * Actualiza last_activity y resetea ping_sent.
 * Si el usuario estaba INACTIVO lo restaura a ACTIVO.
 *
 * Retorna 1 si el estado fue restaurado de INACTIVO a ACTIVO,
 * 0 en cualquier otro caso.
 */
int  um_update_activity(const char *username);
int  um_check_inactivity(void);

/*
 * um_collect_inactivity
 * Revisa todos los clientes bajo un unico mutex y produce dos listas:
 *   needs_ping / ping_socks   — clientes idle >= INACTIVITY_SECS sin PING
 *                               pendiente. Los marca ping_sent=1.
 *   needs_inactive / inactive_socks — clientes con ping_sent=1 cuyo
 *                               tiempo desde ping_time >= ping_wait_secs.
 *                               Los marca INACTIVO.
 */
void um_collect_inactivity(char needs_ping[][MAX_USERNAME],
                           int  ping_socks[],
                           int *n_ping,
                           char needs_inactive[][MAX_USERNAME],
                           int  inactive_socks[],
                           int *n_inactive,
                           int  ping_wait_secs);

/* um_reset_ping: llego PONG, resetear ping_sent y actualizar actividad */
void um_reset_ping(const char *username);

#endif /* USER_MANAGER_H */