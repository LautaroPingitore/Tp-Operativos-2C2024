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
char* QUANTUM;
char* LOG_LEVEL;

int socket_kernel_memoria;
int socket_kernel_cpu_dispatch;
int socket_kernel_cpu_interrupt;

void inicializar_config(char*);


#endif /* KERNEL_H_ */