#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#define MAX_CLIENTES 100

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

extern pthread_mutex_t server_mutex;
extern sem_t sem_clientes;

void handle_signal(int);
void inicializar_config(char*);
void inicializar_archivo(char*, size_t, char*);
void iniciar_archivos();
void* handle_client(void*);
void escuchar_filesystem();
int server_escuchar(t_log*, char*, int);

typedef struct {
    int socket_cliente;
    struct sockaddr_in direccion_cliente;
} t_datos_cliente;


#endif /* FILESYSTEM_H_ */