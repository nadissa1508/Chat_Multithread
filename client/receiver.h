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
 * Archivo: client/receiver.h
 * Descripción: Interfaz del thread receptor de mensajes. Declara
 *              receive_messages(), que corre en segundo plano leyendo
 *              continuamente del socket y mostrando los mensajes
 *              entrantes del servidor al usuario.
 */
#ifndef RECEIVER_H
#define RECEIVER_H

/*
 * receive_messages
 * Función de entrada del thread receptor.
 * Lee mensajes del socket y los imprime en pantalla.
 * Cuando el servidor cierra la conexión, marca g_state.running = 0.
 */
void *receive_messages(void *arg);

#endif // RECEIVER_H 