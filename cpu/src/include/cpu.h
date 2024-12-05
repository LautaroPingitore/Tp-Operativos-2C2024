#ifndef CPU_H_
#define CPU_H_

extern char* IP_MEMORIA;
extern char* PUERTO_MEMORIA;
extern char* PUERTO_ESCUCHA_DISPATCH;
extern char* PUERTO_ESCUCHA_INTERRUPT;
extern char* LOG_LEVEL;
extern char* IP_CPU;

extern t_log *LOGGER_CPU;
extern t_config *CONFIG_CPU;

extern int socket_cpu_dispatch_kernel;
extern int socket_cpu_interrupt_kernel;
extern int socket_cpu_memoria;

extern uint32_t base_pedida;
extern uint32_t limite_pedido;
extern uint32_t valor_memoria;
extern t_instruccion* instruccion_actual;

extern sem_t sem_base; 
extern sem_t sem_limite;
extern sem_t sem_valor_memoria;
extern sem_t sem_instruccion;
extern sem_t sem_mutex_globales;

extern pthread_t hilo_servidor_dispatch;
extern pthread_t hilo_servidor_interrupt;

void inicializar_config(char*);

void iniciar_conexiones();
void* escuchar_cpu();
void* escuchar_cpu_dispatch();
void* escuchar_cpu_interrupt();
int server_escuchar(char*, int);
void* procesar_conexion_dispatch(void*);
void* procesar_conexion_interrupt(void*);

void iniciar_semaforos();
void destruir_semaforos();

#endif /* CPU_H_ */