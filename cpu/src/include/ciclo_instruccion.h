#ifndef CICLO_INSTRUCCION_H_
#define CICLO_INSTRUCCION_H_

#include <include/cpu.h>


//ESTOS STRUCTS VAN EN EL UTILS.
//ESTOS STRUCTS VAN EN EL UTILS.
//ESTOS STRUCTS VAN EN EL UTILS.
//ESTOS STRUCTS VAN EN EL UTILS.
//ESTOS STRUCTS VAN EN EL UTILS.

//LOS IMPLEMENTAMOS ACA PARA PODER LABURAR, PERO ES TRABAJO DE LOS DE KERNEL



typedef enum {
    SUM,
    JNZ,
    SET,
    SUB,
    READ_MEM,
    WRITE_MEM,
    LOG
} nombre_instruccion;

typedef struct {
    nombre_instruccion nombre;  // Tipo de instrucción (SET, SUM, etc.)
    char *parametro1;
    char *parametro2;
    char *parametro3;
    char *parametro4; // Los parametros usados van a depender de la instruccion 
    char *parametro5;
} t_instruccion;

typedef enum {
    EJECUTANDO,
    BLOQUEADO,
    LISTO,
    FINALIZADO
} estado_proceso;

typedef enum {
    INTERRUPCION_SYSCALL,
    INTERRUPCION_BLOQUEO,
    FINALIZACION
} motivo_desalojo;

typedef enum {
    SUCCES,
    ERROR
} finalizacion_proceso;

void ejecutar_ciclo_instruccion(int);
t_instruccion *fetch(uint32_t, uint32_t);
void execute(t_instruccion *, int);
void loguear_y_sumar_pc(t_instruccion*);
void pedir_instruccion_memoria(uint32_t, uint32_t, int);
t_instruccion *deserializar_instruccion(int);
void log_instruccion_ejecutada(nombre_instruccion , char*, char*, char*, char*, char*);
void iniciar_semaforos_etc();
void liberar_instruccion(t_instruccion*);


typedef struct {
    uint32_t tid;
    int prioridad;
} t_tcb;

typedef struct {
    uint32_t program_counter;  // Contador de programa (PC)
    uint32_t AX, BX, CX, DX, EX, FX, GX, HX;  // Registros de propósito general
} t_registros;

typedef struct {
    t_registros *registros;
    motivo_desalojo motivo_desalojo;
    finalizacion_proceso motivo_finalizacion;
} t_contexto_ejecucion;

typedef struct {
    uint32_t pid;  //Identificador Proceso
    uint32_t* tid;  //lista de TIDs
    t_contexto_ejecucion *contexto_ejecucion;
    estado_proceso estado;
} t_pcb;


#endif //CICLO_INSTRUCCION_H_