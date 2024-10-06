#ifndef KERNEL_H_
#define KERNEL_H_

#include <utils/hello.h>

extern t_list* cola_new;
extern t_list* cola_ready;
extern t_list* cola_exit;

extern pthread_mutex_t mutex_cola_new;
extern pthread_mutex_t mutex_cola_ready;
extern pthread_mutex_t mutex_cola_exit;

// PLANIFICADOR LARGO PLAZO
void inicializar_colas_y_mutexs();
t_pcb* crear_pcb(uint32_t, uint32_t*, t_contexto_ejecucion*, t_estado, t_mutex*);
t_tcb* crear_tcb(uint32_t, int, t_estado);
void crear_proceso(char*, int);
void inicializar_proceso(t_pcb*, char*);
void enviar_proceso_a_memoria(int, char*);
void mover_a_ready(t_pcb*);
void mover_a_exit(t_pcb*);
void intentar_inicializar_proceso_de_new();
void process_exit(t_pcb);
void liberar_recursos_proceso(t_pcb*);
uint32_t asignar_pid();
uint32_t* asignar_tids();
t_contexto_ejecucion* asignar_contexto();
t_mutex* asignar_mutexs();
int asignar_prioridad();

// MANEJO HILOS
void thread_create(t_pcb*, char*, int);
void cargar_archivo_pseudocodigo(t_tcb*, char*);
void thread_join(t_pcb*, uint32_t, uint32_t);
t_tcb* buscar_hilo_por_tid(t_pcb*, uint32_t);
void thread_cancel(t_pcb*, uint32_t);
void liberar_recursos_hilo(t_tcb*);
void thread_exit(t_pcb*, uint32_t);
void intentar_mover_a_execute();

#endif /* KERNEL_H_ */