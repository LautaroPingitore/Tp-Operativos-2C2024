#ifndef COMUNICACIONES_H_
#define COMUNICACIONES_H_

#include <include/memoria.h>
#include <include/utils_memoria.h>
#include <include/gestor.h>

// Definición del tipo para los argumentos de la conexión
typedef struct
{
    t_log *log;          // Logger para registrar eventos
    int fd;              // Descriptor de archivo del socket del cliente
    char *server_name;   // Nombre del servidor para logging
} t_procesar_conexion_args;

// Funciones para manejar el servidor y la comunicación con otros módulos
int server_escuchar(t_log *logger, char *server_name, int server_socket);
void enviar_respuesta(int cliente_socket, op_code response); 
static void procesar_conexion_memoria(void *void_args);

#endif // COMUNICACIONES_H_