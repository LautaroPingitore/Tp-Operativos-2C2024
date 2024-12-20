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

extern pthread_t hilo_server_memoria;
extern pthread_t hilo_filesystem;

extern bool mensaje_okey;

void inicializar_programa();
void inicializar_config(char*);
void iniciar_conexiones();
t_list* obtener_particiones_fijas(char**);
void* escuchar_memoria();
int server_escuchar(t_log*, char*, int);
void* manejar_comunicacion_filesystem();
void procesar_conexion_filesystem(int, const char*);
void* procesar_conexion_memoria(void *);

void terminar_memoria();
void destruir_listas();
void destruir_mutexs();

#endif /* MEMORIA_H_ */