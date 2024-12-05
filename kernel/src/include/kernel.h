#ifndef KERNEL_H_
#define KERNEL_H_

extern pthread_t hilo_kernel_memoria;
extern pthread_t hilo_kernel_dispatch;
extern pthread_t hilo_kernel_interrupt;

extern t_log* LOGGER_KERNEL;
extern t_config* CONFIG_KERNEL;

extern char* IP_MEMORIA;
extern char* PUERTO_MEMORIA;
extern char* IP_CPU;
extern char* PUERTO_CPU_DISPATCH;
extern char* PUERTO_CPU_INTERRUPT;
extern int QUANTUM;
extern char* LOG_LEVEL;
extern char* ALGORITMO_PLANIFICACION;

extern int socket_kernel_memoria;
extern int socket_kernel_cpu_dispatch;
extern int socket_kernel_cpu_interrupt;
extern lista_recursos recursos_globales;

void inicializar_config(char*);
void iniciar_conexiones();
void* escuchar_kernel_memoria();
void* escuchar_kernel_cpu_dispatch();
void* escuchar_kernel_cpu_interrupt();
int server_escuchar(char*, int);
void* procesar_conexiones(void*);
void* procesar_conexion_memoria(void*);
void* procesar_conexiones_cpu(void*);

#endif /* KERNEL_H_ */

