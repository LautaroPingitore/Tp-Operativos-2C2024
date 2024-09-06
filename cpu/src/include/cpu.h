#ifndef CPU_H_
#define CPU_H_

#include <utils/hello.h>

char* IP_MEMORIA;
char* PUERTO_MEMORIA;
char* PUERTO_ESCUCHA_DISPATCH;
char* PUERTO_ESCUCHA_INTERRUPT;
char* LOG_LEVEL;

t_log *LOGGER_CPU;
t_config *CONFIG_CPU;

int socket_cpu;
int socket_cpu_dispatch;
int socket_cpu_dispatch_kernel;
int socket_cpu_interrupt;
int socket_cpu_interrupt_kernel;

char* IP_CPU;

void inicializar_config(char*);
void iterator(char*);
int gestionarConexionConKernelDispatch();
int gestionarConexionConKernelInterrupt();

#endif /* CPU_H_ */