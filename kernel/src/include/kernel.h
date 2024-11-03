#ifndef KERNEL_H_
#define KERNEL_H_

#include <utils/hello.h>

t_log* LOGGER_KERNEL;
t_config* CONFIG_KERNEL;

char* IP_MEMORIA;
char* PUERTO_MEMORIA;
char* IP_CPU;
char* PUERTO_CPU_DISPATCH;
char* PUERTO_CPU_INTERRUPT;
char* ALGORITMO_PLANIFICACION;
int QUANTUM;
char* LOG_LEVEL;

int socket_kernel_memoria;
int socket_kernel_cpu_dispatch;
int socket_kernel_cpu_interrupt;
lista_recursos recursos_globales;

void inicializar_config(char*);
void iniciar_comunicaciones(int []);

// LO PONGO PARA QUE NO ESTEN LOS ERRORES
void inicializar_colas_y_mutexs();
void crear_proceso(char*, int, int);

#endif /* KERNEL_H_ */

