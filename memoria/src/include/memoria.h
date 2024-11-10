#ifndef MEMORIA_H_
#define MEMORIA_H_

char* PUERTO_ESCUCHA;
char* IP_FILESYSTEM;
char* PUERTO_FILESYSTEM;
int TAM_MEMORIA;
char* PATH_INSTRUCCIONES;
int RETARDO_RESPUESTA;
char* ESQUEMA;
char* ALGORITMO_BUSQUEDA;
char** PARTICIONES_FIJAS;
char* LOG_LEVEL; 

int socket_memoria;
int socket_memoria_kernel;
int socket_memoria_cpu_dispatch;
int socket_memoria_cpu_interrupt;
int socket_memoria_filesystem;

t_log* LOGGER_MEMORIA;
t_config* CONFIG_MEMORIA;

void inicializar_config(char*);
int gestionarConexionConKernel();
void iterator(char*);
int gestionarConexionConCPUDispatch();
int gestionarConexionConCPUInterrupt();
t_list* obtener_particiones_fijas(char**);

char* IP_MEMORIA;

#endif /* MEMORIA_H_ */