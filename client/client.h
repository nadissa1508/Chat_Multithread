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
 * Archivo: client/client.h
 * Descripción: Define el estado global del cliente (ClientState) y
 *              expone las funciones de conexión TCP y registro ante
 *              el servidor. El estado global es compartido entre el
 *              thread de input y el thread receptor.
 */

#ifndef CLIENT_H
#define CLIENT_H

#include <pthread.h>
#include "../common/protocol.h"

// Estado global de la sesión — compartido entre threads 
typedef struct {
    int          socket_fd;                // socket conectado al servidor  
    char         username[MAX_USERNAME];   // nombre de usuario registrado  
    char         status[MAX_FIELD_LEN];    // status actual del cliente     
    volatile int running;                  // 1 = sesión activa, 0 = salir  
    pthread_t    input_thread;             // thread de input_loop           
} ClientState;

// Instancia global accesible por todos los módulos del cliente 
extern ClientState g_state;

/*
 * client_connect
 * Crea el socket y se conecta al servidor.
 *
 * Retorna el fd del socket, o -1 en error.
 */
int client_connect(const char *server_ip, int port);

/*
 * client_register
 * Envía REGISTER y espera la respuesta del servidor.
 *
 * Retorna  0 si el registro fue aceptado.
 *         -1 si fue rechazado o hubo error de red.
 */
int client_register(int socket_fd, const char *username);

#endif // CLIENT_H 