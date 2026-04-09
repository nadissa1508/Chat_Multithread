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
 * Archivo: server/server.h
 * Descripción: Interfaz pública del módulo de arranque del servidor.
 *              Expone start_server(), que inicializa el socket TCP y
 *              bloquea en el loop de aceptación de conexiones.
 */

#ifndef SERVER_H
#define SERVER_H

/*
 * server.h
 * Arranque del servidor: socket, bind, listen, accept loop.
 */

/*
 * start_server
 * Inicializa el socket del servidor y entra al loop de aceptación.
 * Bloquea indefinidamente (Ctrl+C para salir).
 *
 * Retorna -1 si no pudo inicializar el socket.
 */
int start_server(int port);

#endif /* SERVER_H */