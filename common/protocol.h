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
 * Archivo: protocol.h
 * Descripción: Definiciones del protocolo de comunicación del chat.
 *              Declara las constantes de tipos de mensaje, límites de
 *              tamaño, valores de estado y la estructura Message, así
 *              como la API de parseo y serialización de mensajes.
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

/*
 * Definiciones del protocolo
 *
 * Formato de mensaje:
 *   TIPO|ORIGEN|DESTINO|CONTENIDO\n
 *   - Separador: '|'
 *   - El carácter '|' en CONTENIDO debe escaparse como &pipe;
 *   - Cada mensaje termina con '\n'
 *   - Límite: MAX_MSG_LEN bytes por mensaje
 */

// límites 
#define MAX_FIELD_LEN   64      // tipo, origen, destino              
#define MAX_CONTENT_LEN 4096    // contenido del mensaje              
#define MAX_MSG_LEN     4300    // buffer total: campos + separadores 
#define MAX_USERNAME    50      // longitud máx. de nombre            

// destinos / orígenes especiales 
#define DEST_ALL    "ALL"
#define ORIGIN_SRV  "SERVER"

// tipos de mensaje 

// Registro 
#define MSG_REGISTER        "REGISTER"
#define MSG_REGISTER_OK     "REGISTER_OK"
#define MSG_REGISTER_ERR    "REGISTER_ERR"

// Mensajes
#define MSG_BROADCAST       "BROADCAST"
#define MSG_DIRECT          "MSG"
#define MSG_DELIVER         "MSG_DELIVER"

// Listado e información de usuarios
#define MSG_LIST_USERS      "LIST_USERS"
#define MSG_LIST_USERS_OK   "LIST_USERS_OK"
#define MSG_USER_INFO       "USER_INFO"
#define MSG_USER_INFO_OK    "USER_INFO_OK"
#define MSG_USER_INFO_ERR   "USER_INFO_ERR"

// Status
#define MSG_SET_STATUS      "SET_STATUS"
#define MSG_STATUS_OK       "STATUS_OK"
#define MSG_STATUS_UPDATE   "STATUS_UPDATE"

// Keepalive
#define MSG_PING            "PING"
#define MSG_PONG            "PONG"

// Desconexión
#define MSG_DISCONNECT      "DISCONNECT"
#define MSG_DISCONNECT_OK   "DISCONNECT_OK"

// Error genérico
#define MSG_ERROR           "ERROR"

// valores de status
#define STATUS_ACTIVE   "activo"
#define STATUS_BUSY     "ocupado"
#define STATUS_INACTIVE "inactivo"

// estructura principal

/*
 * Message
 * Representación interna de un mensaje ya parseado.
 * Todos los campos son cadenas terminadas en '\0'.
 */
typedef struct {
    char type[MAX_FIELD_LEN];
    char origin[MAX_FIELD_LEN];
    char destination[MAX_FIELD_LEN];
    char content[MAX_CONTENT_LEN];
} Message;

// API 

/*
 * parse_message
 * Convierte una línea de texto cruda (recibida del socket) en una
 * estructura Message.
 *
 * Parámetros:
 *   raw  - cadena de entrada con formato TIPO|ORIGEN|DESTINO|CONTENIDO\n
 *   msg  - puntero a la estructura Message que se llenará
 *
 * Retorna:
 *    0  si el parseo fue exitoso
 *   -1  si el formato es inválido (campos faltantes o mensaje vacío)
 */
int parse_message(const char *raw, Message *msg);

/*
 * build_message
 * Serializa una estructura Message a una cadena lista para enviar por socket.
 * Agrega '\n' al final automáticamente.
 *
 * Parámetros:
 *   msg    - mensaje a serializar
 *   buffer - buffer de destino (debe tener al menos MAX_MSG_LEN bytes)
 *
 * Retorna:
 *   número de bytes escritos en buffer (sin contar el '\0' final)
 *   -1 si el mensaje es NULL o el buffer es NULL
 */
int build_message(const Message *msg, char *buffer);

/*
 * make_message
 * Helper: rellena una estructura Message y la serializa en un solo paso.
 *
 * Retorna:
 *   número de bytes escritos, o -1 en error
 */
int make_message(const char *type,
                 const char *origin,
                 const char *destination,
                 const char *content,
                 char *buffer);

#endif // PROTOCOL_H 