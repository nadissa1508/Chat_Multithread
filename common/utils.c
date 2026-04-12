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
 * Archivo: utils.c
 * Descripción: Implementación de utilidades compartidas. Incluye el
 *              sistema de logging con marca de tiempo, validación de
 *              nombres de usuario y funciones auxiliares de cadenas
 *              como trim, truncado por palabra y detección de vacíos.
 */

#include "utils.h"
#include "protocol.h"   // MAX_USERNAME 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <ctype.h>
#include <pthread.h>    // mutex para el log 

/* 
 * Escape / Unescape de '|'
 */

int escape_pipe(const char *src, char *dst, size_t dst_size) {
    if (src == NULL || dst == NULL || dst_size == 0) return -1;

    size_t written = 0;
    const char *escape_seq = "&pipe;";
    size_t esc_len = strlen(escape_seq);

    for (size_t i = 0; src[i] != '\0'; i++) {
        if (src[i] == '|') {
            // Verificar que hay espacio para "&pipe;" + '\0' 
            if (written + esc_len >= dst_size) break;
            memcpy(dst + written, escape_seq, esc_len);
            written += esc_len;
        } else {
            if (written + 1 >= dst_size) break;
            dst[written++] = src[i];
        }
    }

    dst[written] = '\0';
    return (int)written;
}

int unescape_pipe(const char *src, char *dst, size_t dst_size) {
    if (src == NULL || dst == NULL || dst_size == 0) return -1;

    size_t written = 0;
    const char *escape_seq = "&pipe;";
    size_t esc_len = strlen(escape_seq);

    for (size_t i = 0; src[i] != '\0'; ) {
        // Comprobar si empieza la secuencia &pipe;
        if (strncmp(src + i, escape_seq, esc_len) == 0) {
            if (written + 1 >= dst_size) break;
            dst[written++] = '|';
            i += esc_len;
        } else {
            if (written + 1 >= dst_size) break;
            dst[written++] = src[i++];
        }
    }

    dst[written] = '\0';
    return (int)written;
}

// Sistema de log
static LogLevel current_log_level = LOG_INFO;

// Mutex para que el log sea thread-safe 
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

void log_set_level(LogLevel level) {
    current_log_level = level;
}

void log_msg(LogLevel level, const char *fmt, ...) {
    if (level < current_log_level) return;

    // Etiquetas por nivel 
    const char *labels[] = { "DEBUG", "INFO ", "WARN ", "ERROR" };
    const char *label = (level <= LOG_ERROR) ? labels[level] : "?????";

    // Timestamp
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char time_buf[20];
    strftime(time_buf, sizeof(time_buf), "%H:%M:%S", t);

    pthread_mutex_lock(&log_mutex);

    fprintf(stderr, "[%s][%s] ", time_buf, label);

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "\n");

    pthread_mutex_unlock(&log_mutex);
}


// Helpers de strings
void trim_newline(char *str) {
    if (str == NULL) return;
    size_t len = strlen(str);
    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
        str[--len] = '\0';
    }
}

int str_is_empty(const char *str) {
    if (str == NULL) return 1;
    while (*str) {
        if (!isspace((unsigned char)*str)) return 0;
        str++;
    }
    return 1;
}

int is_valid_username(const char *username) {
    if (username == NULL) return 0;

    size_t len = strlen(username);
    if (len == 0 || len > MAX_USERNAME) return 0;

    for (size_t i = 0; i < len; i++) {
        char c = username[i];
        // Prohibir '|', espacios y caracteres de control 
        if (c == '|' || isspace((unsigned char)c) || iscntrl((unsigned char)c))
            return 0;
    }

    return 1;
}
// truncate_at_word
int truncate_at_word(const char *src, char *dst, size_t max_len) {
    if (src == NULL || dst == NULL || max_len == 0) return 0;

    size_t src_len = strlen(src);

    if (src_len <= max_len) {
        // No hace falta truncar 
        memcpy(dst, src, src_len + 1);
        return 0;
    }

    // Copiar los primeros max_len bytes y buscar el ultimo espacio 
    memcpy(dst, src, max_len);
    dst[max_len] = '\0';

    // Retroceder desde max_len hasta encontrar un espacio
    size_t cut = max_len;
    while (cut > 0 && dst[cut - 1] != ' ') cut--;

    if (cut > 0) {
        // Cortar en el espacio (eliminarlo tambien para no dejar espacio final)
        dst[cut - 1] = '\0';
    }
    // Si cut == 0 no habia ningun espacio: se queda el corte en max_len 

    return 1;
}