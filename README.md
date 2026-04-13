# Proyecto 01 - Chat Multithread

**Curso:** CC3064 Sistemas Operativos  
**Universidad del Valle de Guatemala - Ciclo 1, 2026**

**Integrantes:**
- Angie Nadissa Vela López - 23764 
- Adrián Ricardo González Muralles - 23152 
- Paula Daniela De León Godoy - 23202 

---

## Descripcion

Aplicacion de chat cliente-servidor desarrollada en C. El servidor atiende multiples clientes de forma concurrente mediante pthreads, asignando un thread independiente por sesion. Un thread monitor adicional gestiona la deteccion de inactividad. El cliente opera con dos threads en paralelo: uno para la entrada del usuario y otro para la recepcion de mensajes del servidor.

---

## Requisitos

- Sistema tipo Linux.
- `gcc` y `make`.
- Soporte de pthreads (se compila con `-pthread`).

---

## Estructura del proyecto

```
Chat_Multithread/
    common/
        protocol.h / protocol.c     Definicion y serializacion del protocolo
        utils.h    / utils.c        Log, validacion de nombres, escape de pipes
    server/
        main.c                      Punto de entrada del servidor
        server.h   / server.c       Socket, bind, listen, accept loop, thread monitor
        client_handler.h / .c       Thread por cliente: registro y despacho de mensajes
        user_manager.h   / .c       Tabla de usuarios compartida, protegida con mutex
        Makefile
    client/
        main.c                      Punto de entrada del cliente
        client.h   / client.c       Conexion TCP y fase de registro
        receiver.h / receiver.c     Thread lector: recibe e imprime mensajes del servidor
        input_handler.h / .c        Thread de entrada: interpreta y envia comandos
        Makefile
```

---

## Protocolo

Formato de cada mensaje: `TIPO|ORIGEN|DESTINO|CONTENIDO\n`

El separador es la barra vertical. Si el contenido incluye ese caracter, debe escaparse como `&pipe;`. El mensaje finaliza con nueva línea. El límite es 4300 bytes por mensaje.

Tipos implementados: `REGISTER`, `REGISTER_OK`, `REGISTER_ERR`, `BROADCAST`, `MSG`, `MSG_DELIVER`, `LIST_USERS`, `LIST_USERS_OK`, `USER_INFO`, `USER_INFO_OK`, `USER_INFO_ERR`, `SET_STATUS`, `STATUS_OK`, `STATUS_UPDATE`, `PING`, `PONG`, `DISCONNECT`, `DISCONNECT_OK`, `ERROR`.

---

## Compilacion

Requiere `gcc` y `make`. Compilar desde el directorio raiz del proyecto:

```
make -C server
make -C client
```

Para limpiar los ejecutables y archivos objeto:

```
make -C server clean
make -C client clean
```

---

## Ejecucion

**Servidor** (en la maquina que actuara como servidor):

```
./server/server <puerto>
```

Ejemplo:

```
./server/server 8080
```

**Cliente** (en cada maquina que se conecte):

```
./client/client <nombre_usuario> <IP_servidor> <puerto>
```

Ejemplo:

```
./client/client alice 192.168.1.10 8080
```


Notas:
- `127.0.0.1` se usa cuando el cliente y servidor corren en la misma maquina.
- El servidor usa TCP/IPv4. Asegurar que el puerto este libre y accesible.
- La longitud maxima es de 50 caracteres.

---

## Comandos del cliente

| Comando                               | Accion                                            |
|--------------------------------------|---------------------------------------------------|
| `<mensaje>`                          | Envia el mensaje a todos los usuarios (broadcast) |
| `/msg <usuario> <mensaje>`           | Envia un mensaje privado al usuario indicado      |
| `/list`                              | Muestra todos los usuarios conectados y su estado |
| `/info <usuario>`                    | Muestra la direccion IP del usuario indicado      |
| `/status <activo\|ocupado\|inactivo>`| Cambia el estado del cliente                      |
| `/help`                              | Muestra la lista de comandos disponibles          |
| `/exit`                              | Cierra la sesion y desconecta del servidor        |

---

## Concurrencia y sincronizacion

La tabla de usuarios (`UserEntry users[]` en `user_manager.c`) es el recurso compartido principal. Todos los accesos a esta estructura ocurren dentro de secciones criticas delimitadas por `pthread_mutex_lock` y `pthread_mutex_unlock` sobre `users_mutex`.

**Condicion de carrera 1: registro simultaneo con el mismo nombre.**
Si dos threads reciben `REGISTER` al mismo tiempo para el mismo nombre, ambos podrian verificar que el nombre esta libre antes de que cualquiera lo inserte, produciendo un duplicado. La verificacion y la insercion son atomicas bajo `users_mutex`.

**Condicion de carrera 2: lectura de socket mientras otro thread lo invalida.**
Un thread puede intentar obtener el `socket_fd` de un usuario para enviarle un mensaje mientras su thread ejecuta `um_remove_user` y cierra ese mismo descriptor. Tanto `um_get_socket` como `um_remove_user` adquieren el mismo mutex, haciendo imposible que ambas operaciones ocurran en paralelo.



---

## Mecanismo de inactividad

El thread monitor revisa la tabla de usuarios cada 10 segundos. El flujo es el siguiente:

1. Si el cliente lleva mas de 30 segundos sin enviar ningun mensaje, el servidor le envia `PING`.
2. El cliente responde automaticamente con `PONG`.
3. Si el servidor no recibe `PONG` en 5 segundos, cambia el estado del cliente a `inactivo` y envia `STATUS_UPDATE`.
4. Cualquier mensaje del cliente resetea el contador y cancela el `PING` pendiente.

Los tiempos se configuran con las constantes `INACTIVITY_SECS` (30 segundos) en `user_manager.h` e `INACTIVITY_CHECK_INTERVAL` (10 segundos) en `server.c`. El servidor espera 5 segundos por `PONG` tras enviar `PING` antes de marcar como inactivo.

---

## Troubleshooting

- **`Address already in use` al iniciar el servidor**: el puerto ya esta en uso. Probar con otro puerto o cerrar el proceso que lo esta usando.
- **`Connection refused` en el cliente**: IP/puerto incorrectos o el servidor no esta corriendo. Verificar que el servidor este escuchando en el puerto indicado.
- **No conecta desde otra maquina**: verificar IP del servidor, reglas de firewall, y que el puerto este abierto/permitido en la red.