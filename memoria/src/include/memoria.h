#ifndef MEMORIA_H_
#define MEMORIA_H_

extern char* PUERTO_ESCUCHA;
extern char* IP_FILESYSTEM;
extern char* PUERTO_FILESYSTEM;
extern int TAM_MEMORIA;
extern char* PATH_INSTRUCCIONES;
extern int RETARDO_RESPUESTA;
extern char* ESQUEMA;
extern char* ALGORITMO_BUSQUEDA;
extern char** PARTICIONES_FIJAS;
extern char* LOG_LEVEL; 
extern char* IP_MEMORIA;

extern int socket_memoria;
extern int socket_memoria_kernel;
extern int socket_memoria_cpu;
extern int socket_memoria_filesystem;

extern t_log* LOGGER_MEMORIA;
extern t_config* CONFIG_MEMORIA;

void inicializar_programa();
void inicializar_config(char*);
void iniciar_conexiones();
t_list* obtener_particiones_fijas(char**);
void* escuchar_memoria();
int server_escuchar(t_log*, char*, int);


#endif /* MEMORIA_H_ */