#ifndef CICLO_INSTRUCCION_H_
#define CICLO_INSTRUCCION_H_

typedef struct {
    uint32_t PID;
    t_contexto_ejecucion* CONTEXTO;
} t_proceso_cpu;

extern t_proceso_cpu* pcb_actual;
extern t_tcb* hilo_actual;

typedef struct {
    uint32_t direccion_fisica;
    uint32_t tamanio;
} t_direcciones_fisicas;

typedef enum {
    SUM,
    READ_MEM,
    WRITE_MEM,
    JNZ,
    SET,
    SUB,
    LOG,
    INST_DUMP_MEMORY,
    INST_IO,
    INST_PROCESS_CREATE,
    INST_THREAD_CREATE,
    INST_THREAD_JOIN,
    INST_THREAD_CANCEL,
    INST_MUTEX_CREATE,
    INST_MUTEX_LOCK,
    INST_MUTEX_UNLOCK,
    INST_THREAD_EXIT,
    INST_PROCESS_EXIT,
    ERROR_INSTRUCCION
} nombre_instruccion;

void ejecutar_ciclo_instruccion();
t_instruccion *fetch(uint32_t, uint32_t);
void execute(t_instruccion *, int, t_tcb*);
void check_interrupt();


void loguear_y_sumar_pc(t_instruccion*);

void liberar_instruccion(t_instruccion*);

void actualizar_contexto_memoria();

#endif //CICLO_INSTRUCCION_H_