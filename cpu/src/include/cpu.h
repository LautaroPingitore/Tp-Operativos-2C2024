#ifndef CPU_H_
#define CPU_H_

#include <utils/hello.h>

char* IP_MEMORIA;
int PUERTO_MEMORIA;
int PUERTO_ESCUCHA_DISPATCH;
int PUERTO_ESCUCHA_INTERRUPT;
char* LOG_LEVEL;

t_log *LOGGER_CPU;
t_config *CONFIG_CPU;

void inicializar_config(char*);


#endif /* CPU_H_ */