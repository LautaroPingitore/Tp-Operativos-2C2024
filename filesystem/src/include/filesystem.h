#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#define MAX_CLIENTES 100

char* PUERTO_ESCUCHA;
char* MOUNT_DIR;
int BLOCK_SIZE;
int BLOCK_COUNT;
int RETARDO_ACCESO_BLOQUE;
char* LOG_LEVEL;

t_log *LOGGER_FILESYSTEM;
t_config *CONFIG_FILESYSTEM;

int socket_filesystem;

extern pthread_mutex_t server_mutex;
extern sem_t sem_clientes;

void inicializar_config(char*);
void iniciar_archivos();
void* handle_client(void*);
void handle_signal(int);

typedef struct {
    int socket_cliente;
    struct sockaddr_in direccion_cliente;
} t_datos_cliente;

char* IP_FILESYSTEM;

#endif /* FILESYSTEM_H_ */