#ifndef PLANIFICADOR_H_
#define PLANIFICADOR_H_

typedef struct {
    int nro; // Numero de prioridad de la cola
    t_list* cola; // Hilos t_tcb*
} t_cola_multinivel;

typedef struct {
    uint32_t tid_esperado;
    t_tcb* hilo_bloqueado;
} t_join;

// COLAS QUE EN LAS CUALES SE GUARDARAN LOS PROCESOS
extern t_list* cola_new;
extern t_list* cola_ready;
extern t_list* cola_exec;
extern t_list* cola_blocked;
extern t_list* cola_exit;
extern t_list* colas_multinivel;

extern t_list* tabla_paths;
extern t_list* tabla_procesos;
// uint32_t pid_actual = 0;
// uint32_t tid_actual = 0;

// SEMAFOFOS QUE PROTEGEN EL ACCESO A CADA COLA
extern pthread_mutex_t mutex_cola_new;
extern pthread_mutex_t mutex_cola_ready;
extern pthread_mutex_t mutex_cola_exit;
extern pthread_mutex_t mutex_cola_blocked;
extern pthread_mutex_t mutex_pid;
extern pthread_mutex_t mutex_tid;
extern pthread_mutex_t mutex_estado;
extern pthread_cond_t cond_estado;

// VARIABLES DE CONTROL
extern uint32_t pid;
extern uint32_t contador_tid;
extern bool cpu_libre;

// PLANIFICADOR LARGO PLAZO
void inicializar_kernel();
t_pcb* crear_pcb(uint32_t, int, t_contexto_ejecucion*, t_estado, char*);
t_tcb* crear_tcb(uint32_t, uint32_t, char*, int, t_estado);
void crear_proceso(char*, int, int);
t_contexto_ejecucion* inicializar_contexto();
void inicializar_proceso(t_pcb*, char*);
void mover_a_ready(t_pcb*);
void mover_hilo_a_ready(t_tcb*);
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

// MANEJO TABLA PATHS
void agregar_path(uint32_t, char*);
char* obtener_path(uint32_t);
void eliminar_path(uint32_t);

// MANEJO HILOS
void thread_create(t_pcb*, char*, int);
void thread_join(t_pcb*, uint32_t, uint32_t);
t_tcb* buscar_hilo_por_tid(t_pcb*, uint32_t);
void thread_cancel(t_pcb*, uint32_t);
void liberar_recursos_hilo(t_pcb*, t_tcb*);
void thread_exit(t_pcb*, uint32_t);
void intentar_mover_a_execute();

// PLANIFICADOR CORTO PLAZO
t_tcb* seleccionar_hilo_por_algoritmo();
t_tcb* obtener_hilo_fifo();
t_tcb* obtener_hilo_x_prioridad();
t_tcb* seleccionar_hilo_multinivel();
void agregar_hilo_a_cola(t_tcb*);
t_cola_multinivel* buscar_cola_multinivel(int);
void eliminar_hilo_cola_multinivel(t_tcb*);
void empezar_quantum(int);
void ejecutar_hilo(t_tcb*);

// ENTRADA SALIDA
void io(t_pcb*, uint32_t, int);

extern t_pcb* obtener_pcb_padre_de_hilo(uint32_t);
void eliminar_pcb_lista(t_list*, uint32_t);
void eliminar_tcb_lista(t_list*, uint32_t);
void agregar_hilo_a_bloqueados(uint32_t, t_tcb*);
void tiene_algun_hilo_bloqueado(uint32_t);
void desbloquear_hilo_bloqueado(t_tcb*);

#endif /* PLANIFICADOR_H_ */