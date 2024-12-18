#ifndef UTILS_HELLO_H_
#define UTILS_HELLO_H_

#include <stdlib.h>
#include <stdio.h>
#include <../commons/log.h>
#include <../commons/string.h>
#include <../commons/config.h>
#include <readline/readline.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <../commons/collections/list.h>
#include <assert.h>
#include <stdint.h>
#include <pthread.h>
#include <semaphore.h>
#include "paquetes.h"
#include "servidores.h"

/**
* @brief Imprime un saludo por consola
* @param quien Modulo desde donde se llama a la funcion
* @return No devuelve nada
*/



// SERVIDORES.h
extern t_log* logger_recibido;

// Definición del tipo para los argumentos de la conexión 
typedef struct {
    t_log *log;          // Logger para registrar eventos
    int fd;              // Descriptor de archivo del socket del cliente
    char *server_name;   // Nombre del servidor para logging 
} t_procesar_conexion_args;

int iniciar_servidor(char*, t_log* , char* , char* );
int esperar_cliente(int, t_log*);
void iterator(char*);
t_log *iniciar_logger(char*, char*);
t_config *iniciar_config(char*, char*);
void terminar_programa(t_config*, t_log*, int[]);
int crear_conexion(char*, char*);
void liberar_socket(int);

// PAQUETES.H
typedef enum{
    HANDSHAKE_kernel,
    HANDSHAKE_memoria,
    HANDSHAKE_cpu,
    HANDSHAKE_interrupt,
    HANDSHAKE_dispatch,
    PEDIR_VALOR_MEMORIA, //SACAR ESTE
    ESCRIBIR_VALOR_MEMORIA,
    ACTUALIZAR_CONTEXTO,
    DEVOLVER_CONTROL_KERNEL,
    LECTURA_MEMORIA,
    SEGF_FAULT,
    MENSAJE,
    PAQUETE,
    PCB,
    PROCESS_CREATE,
    PROCESS_EXIT,
    CONTEXTO,
    INSTRUCCION,
    PEDIDO_INSTRUCCION,
    PEDIDO_WAIT,
    PEDIDO_SIGNAL,
    PEDIDO_READ_MEM,
    PEDIDO_WRITE_MEM,
    ENVIAR_DIRECCION_FISICA,
    SOLICITUD_BASE_MEMORIA,
    SOLICITUD_LIMITE_MEMORIA,
    DUMP_MEMORY,
    IO,
    THREAD_CREATE,
    THREAD_JOIN,
    THREAD_CANCEL,
    MUTEX_CREATE,
    MUTEX_LOCK,
    MUTEX_UNLOCK,
    THREAD_EXIT,
    PROCESO,
    HILO,
    CREAR_ARCHIVO,
    FINALIZACION_QUANTUM,
    SOLICITUD_PROCESO,
    MENSAJE_WRITE_MEM
} op_code;

typedef struct {
	int size;
	void* stream;
} t_buffer;

typedef struct {
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;

t_paquete* recibir_paquete(int);
void* recibir_buffer(int*, int);
int recibir_operacion(int);
char* recibir_mensaje(int);
//void recibir_mensaje(int, t_log*);
void* serializar_paquete(t_paquete*, int);
void enviar_mensaje(char*, int);
void enviar_handshake(int, op_code);
void crear_buffer(t_paquete*);
void agregar_a_paquete(t_paquete*, void*, uint32_t);
int enviar_paquete(t_paquete*, int);
void eliminar_paquete(t_paquete*);
void liberar_conexion(int);
t_paquete* crear_paquete_con_codigo_de_operacion(op_code);


typedef enum {
    INTERRUPCION_SYSCALL,
    INTERRUPCION_BLOQUEO,
    FINALIZACION,
    ESTADO_INICIAL
} motivo_desalojo;

typedef enum {
	INCIAL,
    SUCCES,
    ERROR
} finalizacion_proceso;

typedef struct {
    uint32_t AX;
    uint32_t BX;
    uint32_t CX;
    uint32_t DX;
    uint32_t EX;
    uint32_t FX;
    uint32_t GX;
    uint32_t HX;
} t_registros;

typedef struct {
    t_registros* registros;
    finalizacion_proceso motivo_finalizacion;
} t_contexto_ejecucion;

typedef struct {
    char* nombre;  // Tipo de instrucción (SET, SUM, etc.)
    char* parametro1;
    char* parametro2;
    int parametro3;
} t_instruccion;

typedef enum {
    NEW,
    READY,
    EXECUTE,
    BLOCK_PTHREAD_JOIN,
    BLOCK_MUTEX,
    BLOCK_IO,
    EXIT
} t_estado;

typedef struct {
	uint32_t TID;
	int PRIORIDAD;
	t_estado ESTADO;
    uint32_t PID_PADRE;
    char* archivo;
    uint32_t PC;
    motivo_desalojo motivo_desalojo;
}t_tcb;

typedef struct {
    char* nombre_recurso;
    bool esta_bloqueado;
    uint32_t tid_bloqueador;
    uint32_t pid_hilo;
    t_list* hilos_bloqueados; // Cantidad de hilos bloqueados en este recurso
} t_recurso;

typedef struct {
	uint32_t PID;
	t_list* TIDS;
    int TAMANIO;
	t_contexto_ejecucion* CONTEXTO;
	t_estado ESTADO;
	t_list* MUTEXS; // char*
} t_pcb;

t_tcb* recibir_hilo(int);
t_tcb* deserializar_paquete_tcb(t_buffer*);
t_instruccion* deserializar_instruccion(t_buffer*);
char* deserializar_mensaje(t_buffer*);
uint32_t recibir_pid(int);
uint32_t recibir_uint_memoria(int);

#endif