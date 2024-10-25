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
#include <stdint.h>
#include <pthread.h>
#include <semaphore.h>

/**
* @brief Imprime un saludo por consola
* @param quien Modulo desde donde se llama a la funcion
* @return No devuelve nada
*/

typedef enum{
    HANDSHAKE_consola,
    HANDSHAKE_kernel,
    HANDSHAKE_memoria,
    HANDSHAKE_cpu,
    HANDSHAKE_interrupt,
    HANDSHAKE_dispatch,
    HANDSHAKE_in_out,
    HANDSHAKE_ok_continue,
    ERROROPCODE,
    PEDIR_VALOR_MEMORIA,
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
    INTERRUPCION,
    CONTEXTO,
    PEDIDO_RESIZE,
    INSTRUCCION,
    PEDIDO_INSTRUCCION,
    PEDIDO_WAIT,
    PEDIDO_SIGNAL,
    ENVIAR_INTERFAZ,
    CONEXION_INTERFAZ,
    DESCONEXION_INTERFAZ,
    FINALIZACION_INTERFAZ,
    PEDIDO_SET,
    PEDIDO_READ_MEM,
    PEDIDO_WRITE_MEM,
    PEDIDO_SUB,
    PEDIDO_SUM,
    PEDIDO_JNZ,
    PEDIDO_LOG,
    ENVIAR_PAGINA,
    ENVIAR_DIRECCION_FISICA,
    ENVIAR_INTERFAZ_STDIN,
    ENVIAR_INTERFAZ_STDOUT,
    ENVIAR_INTERFAZ_FS,
    RECIBIR_DATO_STDIN,
    FINALIZACION_INTERFAZ_STDIN,
    FINALIZACION_INTERFAZ_STDOUT,
    PEDIDO_ESCRIBIR_DATO_STDIN,
    PEDIDO_A_LEER_DATO_STDOUT,
    RESPUESTA_STDIN,
    RESPUESTA_DATO_STDOUT,
    PEDIDO_COPY_STRING,
    RESPUESTA_DATO_MOVIN,
    SOLICITUD_BASE_MEMORIA,
    SOLICITUD_LIMITE_MEMORIA,
    MISMO_TAMANIO,
    RESIZE_OK,
    FS_CREATE_DELETE,
    FINALIZACION_INTERFAZ_DIALFS,
    FS_TRUNCATE,
    FS_WRITE_READ,
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
    CREAR_ARCHIVO
} op_code;


typedef enum {
    INTERRUPCION_SYSCALL,
    INTERRUPCION_BLOQUEO,
    FINALIZACION
} motivo_desalojo;

typedef enum {
	INCIAL,
    SUCCES,
    ERROR
} finalizacion_proceso;

typedef struct {
    uint32_t program_counter; // Contador de programa (PC)
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
    t_registros *registros;
    motivo_desalojo motivo_desalojo;
    finalizacion_proceso motivo_finalizacion;
} t_contexto_ejecucion;

typedef struct
{
	int size;
	void* stream;
} t_buffer;

typedef enum {
    SET,
    READ_MEM,
    WRITE_MEM,
    SUM,
    SUB,
    JNZ,
    LOG,
    SEGMENTATION_FAULT //QUE ESTO ESTE ACA DA ERROR EN EL SWITH DE execute PERO SACARLO HABILITA OTROS ERRORES DE que esta undeclared
} nombre_instruccion;

typedef struct {
    nombre_instruccion nombre;  // Tipo de instrucción (SET, SUM, etc.)
    char *parametro1;
    char *parametro2;
    char *parametro3; // ELIMINE LOS OTROS PARAMETROS YA QUE LAS INSTRUCCIONES QUE TENEMOS SOLO USAN HASTA 2 PARAMETROS
} t_instruccion;

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
t_paquete* crear_paquete_con_codigo_operacion(op_code);
void* serializar_paquete(t_paquete*, int);
t_paquete* recibir_paquete_entero(int);
int gestionarConexiones(int, t_log*);





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
	uint32_t PID;
	t_list* TIDS;
	t_contexto_ejecucion* CONTEXTO;
	t_estado ESTADO;
	pthread_mutex_t* MUTEXS; // SE PUEDE SACAR ESTO YA QUE NO SABEMOS IS ES VERDADERAMENTE NECESARIO
} t_pcb;

#endif
