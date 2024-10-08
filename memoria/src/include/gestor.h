#ifndef GESTOR_H_
#define GESTOR_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <inttypes.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <readline/readline.h>

#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include <commons/memory.h>

#include "../../../utils/src/utils/hello.h"

// Estructura para los marcos de memoria
typedef struct
{
    uint32_t numeroMarco; // Número de marco en la memoria
    uint32_t pid;         // ID del proceso que ocupa el marco
} t_marco;

// Estructura para representar el contexto de ejecución
typedef struct
{
    uint32_t AX, BX, CX, DX, EX, FX, GX, HX, PC; // Registros
    uint32_t base;   // Dirección base del proceso
    uint32_t limite; // Tamaño límite del proceso
} t_contexto_ejecucion;

// Estructura para las instrucciones del proceso
typedef struct
{
    char **instrucciones;   // Array de instrucciones del pseudocódigo
    int num_instrucciones;  // Cantidad de instrucciones
} t_instrucciones_proceso;

// Estructura para los procesos en memoria
typedef struct
{
    uint32_t pid;                   // ID del proceso
    char *path;                     // Ruta al archivo de pseudocódigo
    uint32_t tamanio;               // Tamaño del proceso
    t_contexto_ejecucion contexto;  // Contexto de ejecución
    t_instrucciones_proceso instrucciones; // Instrucciones del proceso
} t_proceso_memoria;

// Variables globales
extern char *PUERTO_ESCUCHA_MEMORIA; // Puerto de escucha de memoria
extern int TAM_MEMORIA;               // Tamaño total de la memoria
extern char *PATH_INSTRUCCIONES;      // Ruta a los archivos de instrucciones
extern int RETARDO_RESPUESTA;         // Retardo en las respuestas

extern t_log *LOGGER_MEMORIA;         // Logger para registrar eventos
extern t_config *CONFIG_MEMORIA;      // Configuración de memoria

extern pthread_mutex_t mutex_procesos;           // Mutex para proteger acceso a procesos
extern pthread_mutex_t mutex_memoria_usuario;     // Mutex para proteger el acceso a la memoria de usuario

extern t_list *procesos_totales;          // Lista de procesos en memoria
extern void *memoriaUsuario;               // Puntero a la memoria de usuario
extern uint32_t tamanioMemoria;            // Tamaño total de la memoria de usuario

// Funciones para manejo de memoria y procesos
t_proceso_memoria *crear_proceso(uint32_t pid, uint32_t base, uint32_t limite);
void liberar_proceso(uint32_t pid);
void actualizar_contexto(uint32_t pid, t_contexto_ejecucion nuevo_contexto);
t_contexto_ejecucion *obtener_contexto(uint32_t pid);
char *obtener_instruccion(uint32_t pid, uint32_t pc);

#endif /* GESTOR_H_ */
