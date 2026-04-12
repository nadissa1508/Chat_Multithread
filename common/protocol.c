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
 * Archivo: protocol.c
 * Descripción: Implementación del protocolo de comunicación del chat.
 *              Contiene el parser que convierte líneas de texto crudo
 *              (formato TIPO|ORIGEN|DESTINO|CONTENIDO\n) en estructuras
 *              Message, y el serializador inverso para construir mensajes
 *              listos para enviar por socket.
 */

#include "protocol.h"
#include "utils.h"      /* escape_pipe / unescape_pipe, log_* */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// Internas

/*
 * split_fields
 * Divide una cadena usando '|' como delimitador y guarda hasta
 * `max_fields` punteros en el array `fields`.
 * La cadena `src` es MODIFICADA (strtok_r).
 *
 * Retorna el número real de campos encontrados.
 *
 * Nota: el cuarto campo (contenido) puede contener '|' escapados
 *       como &pipe;, por eso solo partimos exactamente en 4 campos.
 */
static int split_fields(char *src, char *fields[], int max_fields) {
    int count = 0;
    char *saveptr = NULL;
    char *token;

    // Los primeros 3 campos se obtienen normalmente 
    while (count < max_fields - 1) {
        token = strtok_r(src, "|", &saveptr);
        src = NULL;  // para las siguientes llamadas a strtok_r 
        if (token == NULL) break;
        fields[count++] = token;
    }

    // El cuarto campo es el resto de la cadena (puede tener &pipe;) 
    if (count == max_fields - 1 && saveptr != NULL) {
        // Eliminar '\n' o '\r\n' al final si existe 
        size_t len = strlen(saveptr);
        while (len > 0 && (saveptr[len - 1] == '\n' || saveptr[len - 1] == '\r'))
            saveptr[--len] = '\0';
        fields[count++] = saveptr;
    }

    return count;
}


 // parse_message
int parse_message(const char *raw, Message *msg) {
    if (raw == NULL || msg == NULL) return -1;

    // Trabajamos sobre una copia porque split_fields modifica la cadena 
    char copy[MAX_MSG_LEN];
    strncpy(copy, raw, MAX_MSG_LEN - 1);
    copy[MAX_MSG_LEN - 1] = '\0';

    char *fields[4];
    int n = split_fields(copy, fields, 4);

    if (n < 4) {
        // El contenido puede estar vacío pero el campo debe existir 
        if (n == 3) {
            // Aceptamos mensaje con contenido vacío 
            fields[3] = "";
        } else {
            log_warn("parse_message: mensaje mal formado (%d campos): %s", n, raw);
            return -1;
        }
    }

    // Copiar campos a la estructura 
    strncpy(msg->type,        fields[0], MAX_FIELD_LEN - 1);
    strncpy(msg->origin,      fields[1], MAX_FIELD_LEN - 1);
    strncpy(msg->destination, fields[2], MAX_FIELD_LEN - 1);

    // El contenido puede tener &pipe; → desescapar 
    char unescaped[MAX_CONTENT_LEN];
    unescape_pipe(fields[3], unescaped, MAX_CONTENT_LEN);
    strncpy(msg->content, unescaped, MAX_CONTENT_LEN - 1);

    // Asegurar terminación 
    msg->type[MAX_FIELD_LEN - 1]        = '\0';
    msg->origin[MAX_FIELD_LEN - 1]      = '\0';
    msg->destination[MAX_FIELD_LEN - 1] = '\0';
    msg->content[MAX_CONTENT_LEN - 1]   = '\0';

    return 0;
}


 // build_message
int build_message(const Message *msg, char *buffer) {
    if (msg == NULL || buffer == NULL) return -1;

    // Escapar el contenido por si tiene '|'
    char escaped_content[MAX_CONTENT_LEN * 2]; // espacio extra para &pipe;
    escape_pipe(msg->content, escaped_content, sizeof(escaped_content));

    int written = snprintf(buffer, MAX_MSG_LEN,
                           "%s|%s|%s|%s\n",
                           msg->type,
                           msg->origin,
                           msg->destination,
                           escaped_content);

    if (written < 0 || written >= MAX_MSG_LEN) {
        log_warn("build_message: mensaje truncado o error de formato");
        return -1;
    }

    return written;
}


 // make_message
int make_message(const char *type,
                 const char *origin,
                 const char *destination,
                 const char *content,
                 char *buffer) {
    if (type == NULL || origin == NULL || destination == NULL ||
        content == NULL || buffer == NULL) return -1;

    Message msg;
    memset(&msg, 0, sizeof(Message));

    strncpy(msg.type,        type,        MAX_FIELD_LEN   - 1);
    strncpy(msg.origin,      origin,      MAX_FIELD_LEN   - 1);
    strncpy(msg.destination, destination, MAX_FIELD_LEN   - 1);
    strncpy(msg.content,     content,     MAX_CONTENT_LEN - 1);

    return build_message(&msg, buffer);
}