#ifndef KERNEL_H_
#define KERNEL_H_

#include <utils/hello.h>

extern t_list* cola_new;
extern t_list* cola_ready;
extern t_list* cola_exit;

extern pthread_mutex_t mutex_cola_new;
extern pthread_mutex_t mutex_cola_ready;
extern pthread_mutex_t mutex_cola_exit;

void inicializar_colas_y_mutexs();
t_pcb* crear_pcb(uint32_t, uint32_t*, t_contexto_ejecucion*, t_estado, t_mutex*);
t_tcb* crear_tcb(uint32_t, int, t_estado);
void crear_proceso(char*, int);
void inicializar_proceso(t_pcb*);
void enviar_proceso_a_memoria(int, char*);
void mover_a_ready(t_pcb*);
void mover_a_exit(t_pcb*);
void intentar_inicializar_proceso_de_new();
void process_exit(t_pcb);
void liberar_recursos_proceso(t_pcb*);

#endif /* KERNEL_H_ */