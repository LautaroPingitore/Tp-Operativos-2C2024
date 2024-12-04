#include "include/gestor.h"

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

pthread_t hilo_servidor_filesystem;

int main() {
    inicializar_config("filesystem");
    iniciar_archivos();
    cargar_bitmap();
    
    iniciar_conexiones();

    int sockets[] = {socket_filesystem, -1};
    terminar_programa(CONFIG_FILESYSTEM, LOGGER_FILESYSTEM, sockets);
    return 0;
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
    sprintf(bitmap_path, "%s/bitmap.dat", MOUNT_DIR);
    sprintf(bloques_path, "%s/bloques.dat", MOUNT_DIR);

    inicializar_archivo(bitmap_path, (BLOCK_COUNT + 7) / 8, "bitmap.dat");
    inicializar_archivo(bloques_path, BLOCK_COUNT * BLOCK_SIZE, "bloques.dat");
}

void iniciar_conexiones() {
    // SERVER FS 
    socket_filesystem = iniciar_servidor(PUERTO_ESCUCHA,LOGGER_FILESYSTEM,IP_FILESYSTEM,"FILESYSTEM");
    if (socket_filesystem == -1) {
        log_error(LOGGER_FILESYSTEM, "Error al iniciar el servidor. El socket no se pudo crear.");
        exit(EXIT_FAILURE);
    }
    log_info(LOGGER_FILESYSTEM, "Servidor filesystem iniciado y escuchando en el puerto %s", PUERTO_ESCUCHA);

    // HILOS SERVIDORES
    pthread_create(&hilo_servidor_filesystem, NULL, (void*) escuchar_filesystem, NULL);
    //pthread_detach(hilo_servidor_filesystem);

     if (pthread_join(hilo_servidor_filesystem, NULL) != 0) {
         log_error(LOGGER_FILESYSTEM, "Error al esperar la finalización del hilo escuchar_filesystem.");
     } else {
         log_info(LOGGER_FILESYSTEM, "El hilo escuchar_filesystem finalizó correctamente.");
    }
}

void escuchar_filesystem() 
{
    while (server_escuchar(LOGGER_FILESYSTEM, "FILESYSTEM", socket_filesystem));   
}

int server_escuchar(t_log* logger, char* servidor, int socket_server) {
    log_info(logger, "Esperando cliente...");
    int socket_cliente = esperar_cliente(socket_server, logger);

    if (socket_cliente != -1)
	{
		pthread_t hilo;
		t_procesar_conexion_args *args = malloc(sizeof(t_procesar_conexion_args));
        if (!args) {
            log_error(LOGGER_FILESYSTEM, "Error al asignar memoria para las conexiones");
            close(socket_cliente);
            return 0;
        }
		args->log = logger;
		args->fd = socket_cliente;
		args->server_name = servidor;
		pthread_create(&hilo, NULL, (void *)gestionar_conexiones, (void *)args);
		pthread_detach(hilo);
		return 1;
	} else {
        log_error(LOGGER_FILESYSTEM, ":(");
    }
	return 0;
}

void* gestionar_conexiones(void* void_args) {
    t_procesar_conexion_args *args = (t_procesar_conexion_args *)void_args;
    t_log *logger = args->log;
    int socket_cliente = args->fd;

    free(args);

    op_code cod;
    while (1) {
        // Recibir código de operación
        ssize_t bytes_recibidos = recv(socket_cliente, &cod, sizeof(op_code), 0);
        if (bytes_recibidos <= 0) { // El cliente cerró la conexión o hubo un error
            log_error(logger, "El cliente cerró la conexión.");
            break; // Salir del bucle y cerrar el hilo
        }

        switch (cod) {
            case HANDSHAKE_memoria:
                recibir_mensaje(socket_cliente, logger);
                break;

            case MENSAJE:
                recibir_mensaje(socket_cliente, logger);
                break;

            case DUMP_MEMORY:
                log_info(logger, "ENTRO A DUMP_MEMORY");
                t_archivo_dump* archivo = recibir_datos_archivo(socket_cliente);
                int resultado = crear_archivo_dump(archivo->nombre, archivo->contenido, archivo->tamanio_contenido);
                if (resultado == -1) {
                    enviar_mensaje("Error en la creación de archivo", socket_cliente);
                } else {
                    enviar_mensaje("OK", socket_cliente);
                }
                log_info(logger, "## Fin de solicitud - Archivo: %s", archivo->nombre);
                free(archivo->contenido);
                free(archivo->nombre);
                free(archivo);
                break;

            default:
                log_error(logger, "Código de operación desconocido: %d", cod);
                break;
        }
    }

    log_warning(logger, "Finalizando conexión con el cliente.");
    close(socket_cliente); // Cerrar el socket del cliente

    return NULL;
}

t_archivo_dump* recibir_datos_archivo(int socket) {
    t_paquete* paquete = recibir_paquete(socket);
    t_archivo_dump* archivo_recibido = deserializar_archivo_dump(paquete->buffer);
    eliminar_paquete(paquete);
    return archivo_recibido;
}

t_archivo_dump* deserializar_archivo_dump(t_buffer* buffer) {
    t_archivo_dump* archivo_recibido = malloc(sizeof(t_archivo_dump));
    if(archivo_recibido == NULL) {
        printf("Error al asignarle espacio el archivo de dump");
        return NULL;
    }

    void* stream = buffer->stream;
    int desplazamiento = 0;

    uint32_t tamanio_nombre, tamanio_contenido;

    memcpy(&(tamanio_nombre), stream + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    archivo_recibido->tamanio_nombre = tamanio_nombre;

    archivo_recibido->nombre = malloc(tamanio_nombre);
    if(archivo_recibido->nombre == NULL) {
        free(archivo_recibido);
        printf("Error en nombre archivo");
        return NULL;
    }

    memcpy(archivo_recibido->nombre, stream + desplazamiento, tamanio_nombre);
    desplazamiento += tamanio_nombre;

    memcpy(&(tamanio_contenido), stream + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    archivo_recibido->tamanio_contenido = tamanio_contenido;

    archivo_recibido->contenido = malloc(tamanio_contenido);
    if(archivo_recibido->contenido == NULL) {
        free(archivo_recibido);
        printf("Error en contenido archivo");
        return NULL;
    }

    memcpy(archivo_recibido->contenido, stream + desplazamiento, tamanio_contenido);

    return archivo_recibido;
}