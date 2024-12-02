#include "include/gestor.h"

volatile sig_atomic_t server_running = 1;  
// Variable global para controlar el ciclo del servidor
// Manejo de señales con signal(): Se define una función handle_signal para interceptar la señal SIGINT (cuando se presiona Ctrl+C)
// Esto cambia la variable server_running a 0, lo que permite salir del ciclo.
pthread_mutex_t server_mutex = PTHREAD_MUTEX_INITIALIZER;
sem_t sem_clientes;

char* PUERTO_ESCUCHA;
char* MOUNT_DIR;
int BLOCK_SIZE;
int BLOCK_COUNT;
int RETARDO_ACCESO_BLOQUE;
char* LOG_LEVEL;
char* IP_FILESYSTEM;

t_log *LOGGER_FILESYSTEM;
t_config *CONFIG_FILESYSTEM;

int socket_filesystem;

int main(int argc, char *argv[]) {

    signal(SIGINT, handle_signal);

    inicializar_config("filesystem");
    iniciar_archivos();
    cargar_bitmap();

    if (sem_init(&sem_clientes, 0, MAX_CLIENTES) != 0) {
        log_error(LOGGER_FILESYSTEM, "Error al inicializar semáforo");
        exit(EXIT_FAILURE);
    }

    //Iniciar servidor
    socket_filesystem = iniciar_servidor(PUERTO_ESCUCHA,LOGGER_FILESYSTEM,IP_FILESYSTEM,"FILESYSTEM");

    // CICLO PARA ESPERAR CONEXIONES Y CREAR HILOS POR CLIENTE
    while(server_running) {
        // ESPERAR A MEMORIA Y GESTIONAR LA CONEXION
        sem_wait(&sem_clientes);

        int socket_cliente = esperar_cliente(socket_filesystem, LOGGER_FILESYSTEM);
        if (socket_cliente < 0) {
            sem_post(&sem_clientes);
            continue;
        }

        // CREA UN HILO PARA MANEJAR CADA CLIENTE
        pthread_t thread_id;
        t_datos_cliente *datos_cliente = malloc(sizeof(t_datos_cliente));
        if (!datos_cliente) {
            log_error(LOGGER_FILESYSTEM, "Error al asignar memoria para cliente");
            close(socket_cliente);
            sem_post(&sem_clientes);
            continue;
        }

        datos_cliente->socket_cliente = socket_cliente;

        if (pthread_create(&thread_id, NULL, handle_client, datos_cliente) != 0) {
            log_error(LOGGER_FILESYSTEM, "Error al crear hilo");
            free(datos_cliente);
            sem_post(&sem_clientes);
        }
    
        // SEPARA AL HILO PARA QUE SE LIMPIE AUTOMATICAMENTE AL FINALIZAR
        pthread_detach(thread_id);
    
    }

    guardar_bitmap();
    sem_destroy(&sem_clientes);
    int sockets[] = {socket_filesystem, -1};
    terminar_programa(CONFIG_FILESYSTEM, LOGGER_FILESYSTEM, sockets);

    return 0;
}

// Función para manejar la señal SIGINT
void handle_signal(int signal) {
    if (signal == SIGINT) {
        pthread_mutex_lock(&server_mutex);
        server_running = 0;
        log_info(LOGGER_FILESYSTEM, "Servidor cerrándose...");
        pthread_mutex_unlock(&server_mutex);
    }
}

void inicializar_config(char* arg){

    char config_path[256];
    strcpy(config_path, "../");
    strcat(config_path, arg);
    strcat(config_path, ".config");
    
    LOGGER_FILESYSTEM = iniciar_logger("filesystem.config", "FILESYSTEM");
    CONFIG_FILESYSTEM = iniciar_config(config_path,"FILESYSTEM");
    
    PUERTO_ESCUCHA = config_get_string_value(CONFIG_FILESYSTEM, "PUERTO_ESCUCHA");
    MOUNT_DIR = config_get_string_value(CONFIG_FILESYSTEM, "MOUNT_DIR");
    BLOCK_SIZE = config_get_int_value(CONFIG_FILESYSTEM, "BLOCK_SIZE");
    BLOCK_COUNT = config_get_int_value(CONFIG_FILESYSTEM, "BLOCK_COUNT");
    RETARDO_ACCESO_BLOQUE = config_get_int_value(CONFIG_FILESYSTEM, "RETARDO_ACCESO_BLOQUE");
    LOG_LEVEL = config_get_string_value(CONFIG_FILESYSTEM, "LOG_LEVEL");

    IP_FILESYSTEM = config_get_string_value(CONFIG_FILESYSTEM,"IP_FILESYSTEM");
}

void inicializar_archivo(char* path, size_t size) {
    FILE* file = fopen(path, "r+");
    if(!file) { // VERIFICA SI ESTA VACIO
        log_warning(LOGGER_FILESYSTEM, "bitmap.dat no encontrado. Creando...");
        file = fopen(path, "w");
        if (!file) {
            log_error(LOGGER_FILESYSTEM, "Error al crear bitmap.dat");
            exit(EXIT_FAILURE);
        }
        if(size > 0) {
            fseek(file, size - 1, SEEK_SET);
            fputc('\0', file);
        }
    }
    fclose(file);
}

void iniciar_archivos() {
    char bitmap_path[256], bloques_path[256];
    sprintf(bitmap_path, "%s/bitmap.dat", MOUNT_DIR);
    sprintf(bloques_path, "%s/bloques.dat", MOUNT_DIR);

    inicializar_archivo(bitmap_path, (BLOCK_COUNT + 7) / 8);
    inicializar_archivo(bloques_path, BLOCK_COUNT * BLOCK_SIZE);
}

// FUNCION LA CUAL MANEJARA CADA PETICION DE UN HILO
void *handle_client(void *arg) {
    t_datos_cliente *datos_cliente = (t_datos_cliente *) arg;
    int socket_cliente = datos_cliente->socket_cliente;

    while (1) {
        t_paquete* paquete = recibir_paquete_entero(socket_cliente);
        void* stream = paquete->buffer->stream;
        int size = paquete->buffer->size;

        switch (paquete->codigo_operacion) {
            case CREAR_ARCHIVO:
                char* nombre_archivo;
                char* contenido;
                int tamanio;

                memcpy(&nombre_archivo, stream + size, sizeof(strlen(nombre_archivo) + 1));
                size += sizeof(strlen(nombre_archivo) + 1);
                memcpy(&contenido, stream + size, sizeof(strlen(contenido) + 1));
                size += sizeof(strlen(contenido) + 1);
                memcpy(&tamanio, stream + size, sizeof(int));

                if (!nombre_archivo || !contenido || tamanio <= 0) {
                    enviar_mensaje("Datos inválidos", socket_cliente);
                    free(nombre_archivo);
                    free(contenido);
                    break;
                }

                int resultado = crear_archivo_dump(nombre_archivo, contenido, tamanio);
                if (resultado == -1) {
                    enviar_mensaje("Error en la creación de archivo", socket_cliente);
                    free(nombre_archivo);
                    free(contenido);
                } else {
                    enviar_mensaje("OK", socket_cliente);
                }

                log_info(LOGGER_FILESYSTEM, "## Fin de solicitud - Archivo: %s", nombre_archivo);
                free(nombre_archivo);
                free(contenido);
                break;

            case ERROROPCODE:
                log_error(LOGGER_FILESYSTEM, "El cliente se desconectó. Terminando hilo");
                close(socket_cliente);
                free(datos_cliente);
                break;

            default:
                log_warning(LOGGER_FILESYSTEM, "Operacion desconocida");
                break;
        }
    }

    close(socket_cliente);
    free(datos_cliente);
    sem_post(&sem_clientes);
    pthread_exit(NULL);
}