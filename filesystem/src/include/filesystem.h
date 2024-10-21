#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_
#define MAX_CLIENTES 100

#include <utils/hello.h>

char* PUERTO_ESCUCHA;
char* MOUNT_DIR;
char* BLOCK_SIZE;
char* BLOCK_COUNT;
char* RETARDO_ACCESO_BLOQUE;
char* LOG_LEVEL;

t_log *LOGGER_FILESYSTEM;
t_config *CONFIG_FILESYSTEM;

int socket_filesystem;
int socket_filesystem_memoria;

void inicializar_config(char*);
void iterator(char*);
int gestionarConexionConMemoria();
void* handle_client(void*);
void handle_signal(int);

typedef struct {
    int socket_cliente;
    struct sockaddr_in direccion_cliente;
} t_datos_cliente;

char* IP_FILESYSTEM;

#endif /* FILESYSTEM_H_ */