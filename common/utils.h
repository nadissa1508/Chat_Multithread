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
 * Archivo: utils.h
 * Descripción: Utilidades compartidas entre cliente y servidor.
 *              Define el sistema de logging con niveles (DEBUG, INFO,
 *              WARN, ERROR), funciones de validación de usernames y
 *              helpers de manipulación de cadenas.
 */

#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>  /* size_t */

/* 
 * Escape del separador '|'
 * El protocolo usa '|' como delimitador de campos.
 * Si el contenido de un mensaje incluye '|', debe representarse
 * como la secuencia literal "&pipe;" para no romper el parseo.
 */

/*
 * escape_pipe
 * Reemplaza cada '|' en `src` por "&pipe;" en `dst`.
 *
 * Parámetros:
 *   src      - cadena original
 *   dst      - buffer de destino
 *   dst_size - tamaño máximo del buffer de destino
 *
 * Retorna el número de bytes escritos en dst (sin '\0').
 */
int escape_pipe(const char *src, char *dst, size_t dst_size);

/*
 * unescape_pipe
 * Reemplaza cada "&pipe;" en `src` por '|' en `dst`.
 *
 * Retorna el número de bytes escritos en dst (sin '\0').
 */
int unescape_pipe(const char *src, char *dst, size_t dst_size);

/* 
 * Sistema de log
 * Niveles: DEBUG < INFO < WARN < ERROR
 * Todos los mensajes incluyen timestamp y nivel.
 */

typedef enum {
    LOG_DEBUG = 0,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR
} LogLevel;

/*
 * log_set_level
 * Establece el nivel mínimo de log. Mensajes por debajo se descartan.
 * Por defecto: LOG_INFO.
 */
void log_set_level(LogLevel level);

/*
 * log_msg
 * Función base de log (generalmente no se llama directamente).
 */
void log_msg(LogLevel level, const char *fmt, ...);

/* Macros convenientes */
#define log_debug(fmt, ...) log_msg(LOG_DEBUG, fmt, ##__VA_ARGS__)
#define log_info(fmt, ...)  log_msg(LOG_INFO,  fmt, ##__VA_ARGS__)
#define log_warn(fmt, ...)  log_msg(LOG_WARN,  fmt, ##__VA_ARGS__)
#define log_error(fmt, ...) log_msg(LOG_ERROR, fmt, ##__VA_ARGS__)

/* 
 * Helpers de strings
 */

/*
 * trim_newline
 * Elimina '\n' y '\r' del final de la cadena (in-place).
 */
void trim_newline(char *str);

/*
 * str_is_empty
 * Retorna 1 si la cadena es NULL o solo espacios/vacía.
 */
int str_is_empty(const char *str);

/*
 * is_valid_username
 * Retorna 1 si el nombre de usuario es válido:
 *   - No vacío
 *   - Sin '|', espacios ni caracteres de control
 *   - Longitud <= MAX_USERNAME (definido en protocol.h)
 */
int is_valid_username(const char *username);

/*
 * truncate_at_word
 * Copia src en dst truncando en el ultimo espacio si la cadena supera
 * max_len bytes (sin contar el terminador '\0').
 *
 * Si src tiene max_len bytes o menos se copia directamente.
 * Si hay que truncar y no existe ningun espacio en los primeros
 * max_len caracteres, el corte se hace en el byte max_len exacto.
 *
 * Parametros:
 *   src     - cadena de entrada
 *   dst     - buffer de destino (debe tener al menos max_len + 1 bytes)
 *   max_len - numero maximo de caracteres a copiar (sin '\0')
 *
 * Retorna 1 si se trunco, 0 si no fue necesario.
 */
int truncate_at_word(const char *src, char *dst, size_t max_len);

#endif /* UTILS_H */