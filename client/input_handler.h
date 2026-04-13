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
 * Archivo: client/input_handler.h
 * Descripción: Interfaz del manejador de input del usuario. Declara
 *              input_loop(), que lee líneas desde stdin e interpreta
 *              comandos con prefijo '/' o texto plano para broadcast.
 */

#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

/*
 * input_loop
 * Función de entrada del thread de input.
 * Lee stdin en un loop y envía mensajes al servidor.
 * Sale cuando g_state.running == 0 o el usuario escribe /exit.
 */
void *input_loop(void *arg);

/*
 * print_help
 * Imprime la lista de comandos disponibles.
 */
void print_help(void);

#endif // INPUT_HANDLER_H 