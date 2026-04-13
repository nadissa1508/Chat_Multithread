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
 * Archivo: client/input_handler.c
 * Descripción: Interpreta los comandos del usuario y los traduce al
 *              protocolo del servidor. Los comandos disponibles son
 *              /msg, /list, /info, /status, /help y /exit. Cualquier
 *              texto sin prefijo '/' se envía como broadcast a todos
 *              los usuarios conectados.
 */

#include "input_handler.h"
#include "client.h"
#include "../common/protocol.h"
#include "../common/utils.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>

 // print_help
void print_help(void) {
    printf("\n╔══════════════════════════════════════════════╗\n");
    printf("║           COMANDOS DISPONIBLES               ║\n");
    printf("╠══════════════════════════════════════════════╣\n");
    printf("║ <mensaje>                → broadcast         ║\n");
    printf("║ /msg <usuario> <mensaje> → mensaje privado   ║\n");
    printf("║ /list                    → listar usuarios   ║\n");
    printf("║ /info <usuario>          → info de usuario   ║\n");
    printf("║ /status <activo|ocupado|inactivo>            ║\n");
    printf("║                          → cambiar estado    ║\n");
    printf("║ /help                    → mostrar ayuda     ║\n");
    printf("║ /exit                    → salir             ║\n");
    printf("╚══════════════════════════════════════════════╝\n> ");
    fflush(stdout);
}

 // Envío de mensajes al servidor

static void send_broadcast(const char *text) {
    /* Calcular el maximo de contenido disponible:
     * MAX_MSG_LEN - "BROADCAST" - "|" - username - "|ALL|" - "\n"
     * En la practica MAX_CONTENT_LEN (1024) ya es el techo del campo. */
    const size_t max_content = MAX_CONTENT_LEN - 1;

    char safe[MAX_CONTENT_LEN];
    int truncated = truncate_at_word(text, safe, max_content);
    if (truncated) {
        printf("[aviso] El mensaje fue demasiado largo y se corto: \"%s\"\n",
               safe);
        fflush(stdout);
    }

    char buf[MAX_MSG_LEN];
    make_message(MSG_BROADCAST, g_state.username, DEST_ALL, safe, buf);
    send(g_state.socket_fd, buf, strlen(buf), 0);
}

static void send_direct(const char *dest, const char *text) {
    if (str_is_empty(dest) || str_is_empty(text)) {
        printf("[ERROR] Uso: /msg <usuario> <mensaje>\n> ");
        fflush(stdout);
        return;
    }

    const size_t max_content = MAX_CONTENT_LEN - 1;
    char safe[MAX_CONTENT_LEN];
    int truncated = truncate_at_word(text, safe, max_content);
    if (truncated) {
        printf("[aviso] El mensaje fue demasiado largo y se corto: \"%s\"\n",
               safe);
        fflush(stdout);
    }

    char buf[MAX_MSG_LEN];
    make_message(MSG_DIRECT, g_state.username, dest, safe, buf);
    send(g_state.socket_fd, buf, strlen(buf), 0);
}

static void send_list_users(void) {
    char buf[MAX_MSG_LEN];
    make_message(MSG_LIST_USERS, g_state.username, ORIGIN_SRV, "", buf);
    send(g_state.socket_fd, buf, strlen(buf), 0);
}

static void send_user_info(const char *target) {
    if (str_is_empty(target)) {
        printf("[ERROR] Uso: /info <usuario>\n> ");
        fflush(stdout);
        return;
    }
    char buf[MAX_MSG_LEN];
    make_message(MSG_USER_INFO, g_state.username, ORIGIN_SRV, target, buf);
    send(g_state.socket_fd, buf, strlen(buf), 0);
}

static void send_set_status(const char *new_status) {
    if (str_is_empty(new_status)) {
        printf("[ERROR] Uso: /status <activo|ocupado|inactivo>\n> ");
        fflush(stdout);
        return;
    }
    // Validar localmente antes de enviar 
    if (strcmp(new_status, STATUS_ACTIVE)   != 0 &&
        strcmp(new_status, STATUS_BUSY)     != 0 &&
        strcmp(new_status, STATUS_INACTIVE) != 0) {
        printf("[ERROR] Status inválido. Usa: activo, ocupado o inactivo\n> ");
        fflush(stdout);
        return;
    }
    char buf[MAX_MSG_LEN];
    make_message(MSG_SET_STATUS, g_state.username, ORIGIN_SRV, new_status, buf);
    send(g_state.socket_fd, buf, strlen(buf), 0);
}

static void send_disconnect(void) {
    char buf[MAX_MSG_LEN];
    make_message(MSG_DISCONNECT, g_state.username, ORIGIN_SRV, "", buf);
    send(g_state.socket_fd, buf, strlen(buf), 0);
    g_state.running = 0;
}

 // Parseo de comandos

static void process_command(char *line) {
    trim_newline(line);

    if (str_is_empty(line)) {
        printf("> ");
        fflush(stdout);
        return;
    }

    if (line[0] != '/') {
        // Sin slash → broadcast
        send_broadcast(line);
        return;
    }

    // Separar comando del resto: "/cmd arg1 arg2..." 
    char *cmd  = strtok(line, " ");
    char *rest = strtok(NULL, "");   // resto de la línea 

    if (strcmp(cmd, "/help") == 0) {
        print_help();

    } else if (strcmp(cmd, "/list") == 0) {
        send_list_users();

    } else if (strcmp(cmd, "/info") == 0) {
        // rest = "username" 
        char target[MAX_USERNAME] = {0};
        if (rest) {
            trim_newline(rest);
            strncpy(target, rest, MAX_USERNAME - 1);
        }
        send_user_info(target);

    } else if (strcmp(cmd, "/status") == 0) {
        // rest = "activo" | "ocupado" | "inactivo" 
        char st[MAX_FIELD_LEN] = {0};
        if (rest) {
            trim_newline(rest);
            strncpy(st, rest, MAX_FIELD_LEN - 1);
        }
        send_set_status(st);

    } else if (strcmp(cmd, "/msg") == 0) {
        // rest = "username mensaje con espacios" 
        char dest[MAX_USERNAME] = {0};
        char text[MAX_CONTENT_LEN] = {0};

        if (rest) {
            // Primer token de rest = destino, el resto = mensaje
            char *dest_tok = strtok(rest, " ");
            char *msg_tok  = strtok(NULL, "");
            if (dest_tok) strncpy(dest, dest_tok, MAX_USERNAME - 1);
            if (msg_tok)  strncpy(text, msg_tok,  MAX_CONTENT_LEN - 1);
        }
        send_direct(dest, text);

    } else if (strcmp(cmd, "/exit") == 0) {
        send_disconnect();

    } else {
        printf("[ERROR] Comando desconocido: %s  (escribe /help)\n> ", cmd);
        fflush(stdout);
    }
}

 // input_loop — función del thread
void *input_loop(void *arg) {
    (void)arg;

    char line[MAX_CONTENT_LEN];

    print_help();
    printf("> ");
    fflush(stdout);

    while (g_state.running) {
        if (fgets(line, sizeof(line), stdin) == NULL) {
            // EOF (Ctrl+D) — desconectar limpiamente 
            send_disconnect();
            break;
        }

        /* Si la linea no termina en '\n', el usuario escribio mas de lo que
         * cabe en el buffer. Descartar el sobrante del buffer de stdin para
         * que no se procese como si fuera el siguiente comando. */
        if (strchr(line, '\n') == NULL) {
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF) { /* descartar */ }
        }

        if (!g_state.running) break;
        process_command(line);

        if (g_state.running) {
            printf("> ");
            fflush(stdout);
        }
    }

    return NULL;
}