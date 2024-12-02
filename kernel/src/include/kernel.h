#ifndef KERNEL_H_
#define KERNEL_H_

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
void iniciar_comunicaciones(int []);


#endif /* KERNEL_H_ */

