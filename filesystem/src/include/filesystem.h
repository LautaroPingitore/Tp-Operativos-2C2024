#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#include "../../utils/src/utils/include/hello.h"

extern char* PUERTO_ESCUCHA;
extern char* MOUNT_DIR;
extern int BLOCK_SIZE;
extern int BLOCK_COUNT;
extern int RETARDO_ACCESO_BLOQUE;
extern char* LOG_LEVEL;
extern char* IP_FILESYSTEM;

extern t_log *LOGGER_FILESYSTEM;
extern t_config *CONFIG_FILESYSTEM;

extern int socket_filesystem;
extern pthread_t hilo_servidor_filesystem;


typedef struct {
    char* nombre;
    uint32_t tamanio_nombre;
    char* contenido;
    uint32_t tamanio_contenido;
} t_archivo_dump;

typedef struct {
    int socket_cliente;
    struct sockaddr_in direccion_cliente;
} t_datos_cliente;

void inicializar_config(char*);
void inicializar_archivo(char*, size_t, char*);
void iniciar_archivos();
void iniciar_conexiones();
void* escuchar_filesystem();
int server_escuchar(t_log*, char* , int);
void* gestionar_conexiones(void*);


t_archivo_dump* recibir_datos_archivo(int);
t_archivo_dump* deserializar_archivo_dump(t_buffer*);

#endif /* FILESYSTEM_H_ */