#ifndef PLANIFICADOR_H_
#define PLANIFICADOR_H_

// extern t_list* cola_new;
// extern t_list* cola_ready;
// extern t_list* cola_exit;
// extern t_list* cola_nivel_1;
// extern t_list* cola_nivel_2;
// extern t_list* cola_nivel_3;

// extern pthread_mutex_t mutex_cola_new;
// extern pthread_mutex_t mutex_cola_ready;
// extern pthread_mutex_t mutex_cola_exit;

const int INT_MAX = 200;

// PLANIFICADOR LARGO PLAZO
void inicializar_kernel();
t_pcb* crear_pcb(uint32_t, int, t_contexto_ejecucion*, t_estado, pthread_mutex_t*);
t_tcb* crear_tcb(uint32_t, uint32_t, char*, int, t_estado);
void crear_proceso(char*, int, int);
t_contexto_ejecucion* inicializar_contexto();
void inicializar_proceso(t_pcb*, char*);
void mover_a_ready(t_pcb*);
void mover_a_exit(t_pcb*);
void intentar_inicializar_proceso_de_new();
void serializar_paquete_para_memoria(t_paquete*, int, char*);
void process_exit(t_pcb*);
void liberar_recursos_proceso(t_pcb*);
uint32_t asignar_pid();
uint32_t asignar_tid(t_pcb*);
t_contexto_ejecucion* asignar_contexto();
pthread_mutex_t* asignar_mutexs();
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

// PLANIFICADOR CORTO PLAZO
t_tcb* seleccionar_hilo_por_algoritmo();
t_tcb* obtener_hilo_fifo();
t_tcb* obtener_hilo_x_prioridad();
t_tcb* seleccionar_hilo_multinivel();

// ENTRADA SALIDA
void io(t_pcb*, uint32_t, int);

void eliminar_pcb_lista(t_list*, uint32_t);
void esperar_a_que_termine(t_tcb*, t_tcb*);
void bloquear_hilo_actual(t_tcb*);
void desbloquear_hilo_actual(t_tcb*);
int enviar_proceso_a_cpu(t_pcb*);

#endif /* PLANIFICADOR_H_ */