#ifndef MEMORIA_H_
#define MEMORIA_H_

#include <utils/hello.h>

char* PUERTO_ESCUCHA;
char* IP_FILESYSTEM;
char* PUERTO_FILESYSTEM;
char* TAM_MEMORIA;
char* PATH_INSTRUCCIONES;
char* RETARDO_RESPUESTA;
char* ESQUEMA;
char* ALGORITMO_BUSQUEDA;
char** PARTICIONES;
char* LOG_LEVEL; 

int socket_memoria;
int socket_memoria_kernel;
int socket_memoria_cpu_dispatch;

t_log* LOGGER_MEMORIA;
t_config* CONFIG_MEMORIA;

void inicializar_config(char*);
int gestionarConexionConKernel();
void iterator(char*);
int gestionarConexionConCPUDispatch();

char* IP_MEMORIA;

#endif /* MEMORIA_H_ */