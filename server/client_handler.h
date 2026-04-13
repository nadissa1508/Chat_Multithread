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
 * Archivo: server/client_handler.h
 * Descripción: Interfaz del manejador de sesiones de cliente. Declara
 *              handle_client() (función principal del thread por cliente),
 *              send_to_user() para envío directo y broadcast_message()
 *              para difusión a todos los usuarios conectados.
 */

#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include <netinet/in.h>

// Argumento que server.c pasa al thread al crearlo 
typedef struct {
    int socket_fd;                  // socket del cliente recién conectado
    struct sockaddr_in client_addr; // dirección IP y puerto del cliente
} ClientArgs;

/*
 * handle_client
 * Función de entrada del thread para cada cliente.
 * Gestiona toda la sesión: registro, mensajes, desconexión.
 *
 * El thread libera `arg` al finalizar.
 */
void *handle_client(void *arg);

/*
 * send_to_user
 * Envía un mensaje ya serializado a un usuario por nombre.
 * Thread-safe: obtiene el socket vía user_manager.
 *
 * Retorna bytes enviados, o -1 si el usuario no existe o hubo error.
 */
int send_to_user(const char *username, const char *msg_buffer);

/*
 * broadcast_message
 * Envía `msg_buffer` a todos los usuarios conectados excepto `exclude`
 * (puede ser NULL para incluir a todos).
 */
void broadcast_message(const char *msg_buffer, const char *exclude);

#endif // CLIENT_HANDLER_H 