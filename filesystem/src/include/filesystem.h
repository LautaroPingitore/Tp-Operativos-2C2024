#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#include <utils/hello.h>

int PUERTO_ESCUCHA;
char* MOUNT_DIR;
int BLOCK_SIZE;
int BLOCK_COUNT;
int RETARDO_ACCESO_BLOQUE;
char* LOG_LEVEL;

t_log *LOGGER_FILESYSTEM;
t_config *CONFIG_FILESYSTEM;

void inicializar_config(char*);

#endif /* FILESYSTEM_H_ */