#ifndef CICLO_INSTRUCCION_H_
#define CICLO_INSTRUCCION_H_

#include "include/cpu.h"
#include "include/MMU.h"
#include "include/instrucciones.h"


//ESTOS STRUCTS VAN EN EL UTILS.
//ESTOS STRUCTS VAN EN EL UTILS.
//ESTOS STRUCTS VAN EN EL UTILS.
//ESTOS STRUCTS VAN EN EL UTILS.
//ESTOS STRUCTS VAN EN EL UTILS.

//LOS IMPLEMENTAMOS ACA PARA PODER LABURAR, PERO ES TRABAJO DE LOS DE KERNEL

typedef enum {
    EJECUTANDO,
    BLOQUEADO,
    LISTO,
    FINALIZADO
} estado_proceso;



typedef struct {
    uint32_t tid;
    int prioridad;
} t_tcb;

typedef struct {
    uint32_t direccion_fisica;
    uint32_t tamanio;
} t_direcciones_fisicas;


typedef struct {
    uint32_t pid;  //Identificador Proceso
    uint32_t* tid;  //lista de TIDs
    t_contexto_ejecucion *contexto_ejecucion;
    estado_proceso estado;
} t_pcb;

void ejecutar_ciclo_instruccion(int);
t_instruccion *fetch(uint32_t, uint32_t);
void execute(t_instruccion *, int);
void loguear_y_sumar_pc(t_instruccion*);
void pedir_instruccion_memoria(uint32_t, uint32_t, int);
t_instruccion *deserializar_instruccion(int);
void log_instruccion_ejecutada(nombre_instruccion , char*, char*, char*);
void iniciar_semaforos_etc();
void liberar_instruccion(t_instruccion*);
char *instruccion_to_string(nombre_instruccion);
void check_interrupt();
bool recibir_interrupcion(int);
void actualizar_contexto_memoria();
void enviar_contexto_memoria(uint32_t, t_registros*, uint32_t, int);
void devolver_control_al_kernel();

#endif //CICLO_INSTRUCCION_H_