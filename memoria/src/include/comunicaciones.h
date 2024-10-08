#ifndef COMUNICACIONES_H_
#define COMUNICACIONES_H_

#include "./gestor.h"
#include "./utils_memoria.h"

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

// Funciones para manejar las operaciones relacionadas con procesos
void recibir_inicializar_proceso(uint32_t *pid, uint32_t *base, uint32_t *limite, int cliente_socket);
void recibir_finalizar_proceso(uint32_t *pid_a_finalizar, int cliente_socket);
void recibir_pedido_instruccion(uint32_t *pid, uint32_t *pc, int cliente_socket);
t_instruccion* obtener_instruccion(uint32_t pid, uint32_t pc);
void liberar_proceso(uint32_t pid);

#endif // COMUNICACIONES_H_
