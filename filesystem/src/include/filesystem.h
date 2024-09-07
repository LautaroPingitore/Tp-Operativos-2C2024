#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

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

char* IP_FILESYSTEM;

#endif /* FILESYSTEM_H_ */