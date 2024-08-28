#ifndef KERNEL_H_
#define KERNEL_H_

#include <utils/hello.h>

t_log* LOGGER_KERNEL;
t_config* CONFIG_KERNEL;

char* IP_MEMORIA;
int PUERTO_MEMORIA;
char* IP_CPU;
int PUERTO_CPU_DISPATCH;
int PUERTO_CPU_INTERRUPT;
char* ALGORITMO_PLANIFICACION;
int QUANTUM;
char* LOG_LEVEL;

void inicializar_config(char*);




#endif /* KERNEL_H_ */