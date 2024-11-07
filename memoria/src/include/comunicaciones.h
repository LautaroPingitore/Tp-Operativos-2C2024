#ifndef COMUNICACIONES_H_
#define COMUNICACIONES_H_

#include <include/memoria.h>

void *memoriaUsuario;

// Definición del tipo para los argumentos de la conexión
typedef struct
{
    t_log *log;          // Logger para registrar eventos
    int fd;              // Descriptor de archivo del socket del cliente
    char *server_name;   // Nombre del servidor para logging
} t_procesar_conexion_args;

// Funciones para manejar el servidor y la comunicación con otros módulos
int server_escuchar(t_log *, char *, int );
void enviar_respuesta(int, char*); 
static void procesar_conexion_memoria(void *);

void recibir_pedido_instruccion(uint32_t*, uint32_t*, int);
t_instruccion* obtener_instruccion(uint32_t, uint32_t);
void enviar_instruccion(int, t_instruccion*);
void recibir_set(uint32_t*, uint32_t*, uint32_t*, int);
t_contexto_ejecucion*  obtener_contexto(uint32_t);

void recibir_read_mem(uint32_t* , uint32_t*, int);
void recibir_write_mem(uint32_t*, uint32_t*, uint32_t*, int);
void recibir_sub(uint32_t*, uint32_t*, uint32_t*, int);
void recibir_sum(uint32_t*, uint32_t*, uint32_t*, int);
void recibir_jnz(uint32_t*, uint32_t* , uint32_t* , int);
void cambiar_pc(uint32_t, uint32_t);
void recibir_log(char [256], int);

typedef struct {
    uint32_t pid;
    uint32_t tid;
    uint32_t base; //OJO CON LOS TIPOS DE DATOS
    uint32_t limite;
} t_proceso_memoria;

#endif //COMUNICACIONES_H_