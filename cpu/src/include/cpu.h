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

char* IP_CPU;

void inicializar_config(char*);
void iterator(char*);


#endif /* CPU_H_ */