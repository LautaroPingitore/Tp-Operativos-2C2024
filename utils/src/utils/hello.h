#ifndef UTILS_HELLO_H_
#define UTILS_HELLO_H_

#include <stdlib.h>
#include <stdio.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <readline/readline.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <commons/collections/list.h>
#include <assert.h>
#include <pthread.h>
#include <semaphore.h>

/**
* @brief Imprime un saludo por consola
* @param quien Modulo desde donde se llama a la funcion
* @return No devuelve nada
*/


typedef enum
{
	MENSAJE,
	PAQUETE
}op_code;

typedef struct
{
	int size;
	void* stream;
} t_buffer;

typedef struct
{
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;

extern t_log* logger;

void* recibir_buffer(int*, int);

int iniciar_servidor(char*, t_log*,char*,char*);
int esperar_cliente(int, t_log*);
void recibir_mensaje(int, t_log*);
int recibir_operacion(int);
t_log* iniciar_logger(char*, char*);
t_config* iniciar_config(char*,char*);
int crear_conexion(char* ip, char* puerto);
void enviar_mensaje(char* mensaje, int socket_cliente);
t_paquete* crear_paquete(void);
void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
int enviar_paquete(t_paquete* paquete, int socket_cliente);
void liberar_socket(int socket_cliente);
void eliminar_paquete(t_paquete* paquete);
t_list* recibir_paquete(int);
void paquete(int, t_log*);
void terminar_programa(t_config*, t_log*, int []);

int gestionarConexiones(int, t_log*);

// NO SE SI ESTO IRIA ACA PERO LO PONGO PARA QUE NO TIRE ERRORES
typedef struct {
    uint32_t program_counter;  // Contador de programa (PC)
    uint32_t  AX, BX, CX, DX ,EX, FX, GX, HX; // PUEDE REMPLAZARCE CON UN registros[8]
} t_registros;

typedef enum {
	ESTADO_INICIAL,
    INTERRUPCION_SYSCALL,
    INTERRUPCION_BLOQUEO,
    FINALIZACION
} motivo_desalojo;

typedef enum {
	INCIAL,
    SUCCES,
    ERROR
} finalizacion_proceso;

// ===========================
// typedef struct {
// 	int id;
// 	int contador;
// } t_mutex;

typedef enum {
    NEW,
    READY,
    EXECUTE,
    BLOCK,
    EXIT
} t_estado;

typedef struct {
	uint32_t TID;
	int PRIORIDAD;
	t_estado ESTADO;
}t_tcb;

typedef struct {
    t_registros *registros;
    motivo_desalojo motivo_desalojo;
    finalizacion_proceso motivo_finalizacion;
} t_contexto_ejecucion;

typedef struct {
	uint32_t PID;
	uint32_t* TIDS;
	t_contexto_ejecucion* CONTEXTO;
	t_estado ESTADO;
	pthread_mutex_t* MUTEXS;
} t_pcb;

#endif