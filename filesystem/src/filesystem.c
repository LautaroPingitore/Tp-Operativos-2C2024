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

int main() {

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
    log_info(LOGGER_FILESYSTEM, "Servidor filesystem iniciado y escuchando en el puerto %s", PUERTO_ESCUCHA);

    // CICLO PARA ESPERAR CONEXIONES Y CREAR HILOS POR CLIENTE
    while(server_running) {
        // ESPERAR A MEMORIA Y GESTIONAR LA CONEXION
        sem_wait(&sem_clientes);

        int socket_cliente = esperar_cliente(socket_filesystem, LOGGER_FILESYSTEM);

        // CREA UN HILO PARA MANEJAR CADA CLIENTE
        pthread_t thread_id;
        t_datos_cliente *datos_cliente = malloc(sizeof(t_datos_cliente));
        if (!datos_cliente) {
            log_error(LOGGER_FILESYSTEM, "Error al asignar memoria para cliente");
            close(socket_cliente);
            sem_post(&sem_clientes);
            continue;
        }
        log_info(LOGGER_FILESYSTEM, "Se creo el hilo para manejar el cliente");

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

void inicializar_archivo(char* path, size_t size, char* nombre) {
    FILE* file = fopen(path, "r+");
    if(!file) { // VERIFICA SI ESTA VACIO
        log_warning(LOGGER_FILESYSTEM, "%s no encontrado. Creando...", nombre);
        file = fopen(path, "w");
        if (!file) {
            log_error(LOGGER_FILESYSTEM, "Error al crear %s", nombre);
            exit(EXIT_FAILURE);
        }
        if(size > 0) {
            fseek(file, size - 1, SEEK_SET);
            fputc('\0', file);
        }
    }
    log_info(LOGGER_FILESYSTEM, "El archivo ya se encuentra creado");
    fclose(file);
}

void iniciar_archivos() {
    char bitmap_path[256], bloques_path[256];
    sprintf(bitmap_path, "../%s/bitmap.dat", MOUNT_DIR);
    sprintf(bloques_path, "../%s/bloques.dat", MOUNT_DIR);

    inicializar_archivo(bitmap_path, (BLOCK_COUNT + 7) / 8, "bitmap.dat");
    inicializar_archivo(bloques_path, BLOCK_COUNT * BLOCK_SIZE, "bloques.dat");
}

// FUNCION LA CUAL MANEJARA CADA PETICION DE UN HILO
void *handle_client(void *arg) {
    t_datos_cliente *datos_cliente = (t_datos_cliente *)arg;
    int socket_cliente = datos_cliente->socket_cliente;

    char* hola = "Hola FILESYSTEM, soy Memoria";
    size_t tam = sizeof(strlen(hola) + 1);

    log_info(LOGGER_FILESYSTEM, "TmzxZxZXxansadada = %d", (int*)tam);
    log_info(LOGGER_FILESYSTEM, "Socket cliente: %d", socket_cliente);

    while (1) {
        log_info(LOGGER_FILESYSTEM, "Esperando un paquete");

        t_paquete *paquete = recibir_paquete_entero(socket_cliente);
        if (!paquete) {
            log_error(LOGGER_FILESYSTEM, "Error al recibir paquete. Terminando hilo.");
            close(socket_cliente);
            free(datos_cliente);
            sem_post(&sem_clientes);
            pthread_exit(NULL);
        }

        void *stream = paquete->buffer->stream;
        int offset = paquete->buffer->size;

        log_info(LOGGER_FILESYSTEM, "%d", paquete->buffer->size);

        switch (paquete->codigo_operacion) {
            case CREAR_ARCHIVO: {
                uint32_t nombre_size, contenido_size;
                char *nombre_archivo, *contenido;
                int tamanio;

                // Deserializar nombre del archivo
                memcpy(&nombre_size, stream + offset, sizeof(uint32_t));
                offset += sizeof(uint32_t);
                nombre_archivo = malloc(nombre_size);
                memcpy(nombre_archivo, stream + offset, nombre_size);
                offset += nombre_size;

                // Deserializar contenido
                memcpy(&contenido_size, stream + offset, sizeof(uint32_t));
                offset += sizeof(uint32_t);
                contenido = malloc(contenido_size);
                memcpy(contenido, stream + offset, contenido_size);
                offset += contenido_size;

                // Deserializar tamaño del archivo
                memcpy(&tamanio, stream + offset, sizeof(int));
                offset += sizeof(int);

                if (!nombre_archivo || !contenido || tamanio <= 0) {
                    enviar_mensaje("Datos inválidos", socket_cliente);
                    free(nombre_archivo);
                    free(contenido);
                    break;
                }

                int resultado = crear_archivo_dump(nombre_archivo, contenido, tamanio);
                if (resultado == -1) {
                    enviar_mensaje("Error en la creación de archivo", socket_cliente);
                } else {
                    enviar_mensaje("OK", socket_cliente);
                }

                log_info(LOGGER_FILESYSTEM, "## Fin de solicitud - Archivo: %s", nombre_archivo);
                free(nombre_archivo);
                free(contenido);
                break;
            }

            case MENSAJE: {
                log_info(LOGGER_FILESYSTEM, "ENTRO A LA SWITCHEACION DE MENSAJE :)");
                char* respuesta;
                memcpy(&respuesta, stream + offset, sizeof(30));
                log_info(LOGGER_FILESYSTEM, "%s", respuesta);
                break;
            }

            case ERROROPCODE:
                log_error(LOGGER_FILESYSTEM, "El cliente se desconectó. Terminando hilo");
                close(socket_cliente);
                free(datos_cliente);
                sem_post(&sem_clientes);
                pthread_exit(NULL);

            default:
                log_warning(LOGGER_FILESYSTEM, "Operación desconocida");
                break;
        }

        eliminar_paquete(paquete);
    }

    close(socket_cliente);
    free(datos_cliente);
    sem_post(&sem_clientes);
    pthread_exit(NULL);
}