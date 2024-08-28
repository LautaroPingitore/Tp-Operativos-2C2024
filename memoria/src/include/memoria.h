#ifndef MEMORIA_H_
#define MEMORIA_H_

#include <utils/hello.h>

int PUERTO_ESCUCHA;
char* IP_FILESYSTEM;
int PUERTO_FILESYSTEM;
int TAM_MEMORIA;
char* PATH_INSTRUCCIONES;
int RETARDO_RESPUESTA;
char* ESQUEMA;
char* ALGORITMO_BUSQUEDA;
char** PARTICIONES;
char* LOG_LEVEL; 

t_log* LOGGER_MEMORIA;
t_config* CONFIG_MEMORIA;

void inicializar_config(char*);

#endif /* MEMORIA_H_ */