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


void inicializar_config(char*);

void iniciar_comunicaciones(int[]);
void* procesar_conexion(void*);

#endif /* CPU_H_ */