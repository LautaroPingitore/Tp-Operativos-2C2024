#ifndef PLANIFICADOR_H_
#define PLANIFICADOR_H_

typedef struct {
    int nro; // Numero de prioridad de la cola
    t_list* cola; // Hilos t_tcb*
} t_cola_multinivel;

typedef struct {
    uint32_t tid_esperado;
    uint32_t pid_hilo;
    t_tcb* hilo_bloqueado;
} t_join;

typedef struct {
    uint32_t pid_hilo;
    uint32_t tid;
    int milisegundos;
    bool se_cancelo;
} t_io;

// COLAS QUE EN LAS CUALES SE GUARDARAN LOS PROCESOS
extern t_list* cola_new;
extern t_list* cola_ready;
extern t_list* cola_exec;
extern t_list* cola_blocked_join;
extern t_list* cola_blocked_mutex;
extern t_list* cola_blocked_io;
extern t_list* cola_exit;
extern t_list* colas_multinivel;

extern t_list* tabla_paths;
extern t_list* tabla_procesos;
extern t_list* recursos_globales;
// uint32_t pid_actual = 0;
// uint32_t tid_actual = 0;

// SEMAFOFOS QUE PROTEGEN EL ACCESO A CADA COLA
extern pthread_mutex_t mutex_cola_new;
extern pthread_mutex_t mutex_cola_ready;
extern pthread_mutex_t mutex_cola_exec;
extern pthread_mutex_t mutex_cola_exit;
extern pthread_mutex_t mutex_cola_blocked;
extern pthread_mutex_t mutex_pid;
extern pthread_mutex_t mutex_tid;
extern pthread_mutex_t mutex_cola_multinivel;
extern pthread_cond_t cond_estado;

extern sem_t sem_io;

// VARIABLES DE CONTROL
extern uint32_t pid;
extern uint32_t contador_tid;
extern bool cpu_libre;
extern bool termino_programa;

// PLANIFICADOR LARGO PLAZO
void inicializar_kernel();
t_pcb* crear_pcb(uint32_t, int, t_contexto_ejecucion*, t_estado, char*, int);
t_tcb* crear_tcb(uint32_t, uint32_t, char*, int, t_estado);
void crear_proceso(char*, int, int);
t_contexto_ejecucion* inicializar_contexto();
int inicializar_proceso(t_pcb*, char*);
void mover_a_ready(t_pcb*);
void mover_hilo_a_ready(t_tcb*);
void mover_a_exit(t_pcb*);
void intentar_inicializar_proceso_de_new();
void serializar_paquete_para_memoria(t_paquete*, int, char*);
void process_exit(t_pcb*);
void process_cancel(t_pcb*);
void terminar_hilos_proceso(t_pcb*);
void eliminar_hilos_ready(uint32_t);
void eliminar_hilos_block_mutex(uint32_t);
void eliminar_hilos_block_join(uint32_t);
void eliminar_hilos_block_io(t_pcb*);
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
bool hay_algo_en_cmn();
t_tcb* seleccionar_hilo_multinivel();
void agregar_hilo_a_cola(t_tcb*);
t_cola_multinivel* buscar_cola_multinivel(int);
void eliminar_hilo_cola_multinivel(t_tcb*);
void expandir_lista_hasta_indice(int);
void empezar_quantum(int);
void ejecutar_hilo(t_tcb*);

// ENTRADA SALIDA
void io(t_pcb*, uint32_t, int);
void empezar_io(uint32_t, uint32_t, int);
void* ejecutar_temporizador_io(void*);
void agregar_hilo_a_bloqueados_io(t_io*);
void desbloquear_hilo_bloqueado_io(uint32_t, uint32_t);

extern t_pcb* obtener_pcb_padre_de_hilo(uint32_t);
void eliminar_pcb_lista(t_list*, uint32_t);
void eliminar_tcb_lista(t_list*, uint32_t);
void agregar_hilo_a_bloqueados(uint32_t, t_tcb*);
void tiene_algun_hilo_bloqueado_join(uint32_t, uint32_t);
void desbloquear_hilo_bloqueado_join(t_tcb*);

// FUNCIONES PARA CUANDO SE LLENA EL FS
void terminar_procesos();
void eliminar_hilos_lista(t_pcb*);
void eliminar_tcb(t_tcb*);


void process_cancel(t_pcb*);
void terminar_hilos_proceso(t_pcb*);
void eliminar_hilos_ready(uint32_t);
void eliminar_hilos_block_mutex(uint32_t);
void eliminar_hilos_block_join(uint32_t);
void eliminar_hilos_block_io(t_pcb*);

#endif /* PLANIFICADOR_H_ */