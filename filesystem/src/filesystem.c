#include "include/gestor.h"

volatile sig_atomic_t server_running = 1;  // Variable global para controlar el ciclo del servidor
// Manejo de señales con signal(): Se define una función handle_signal para interceptar la señal SIGINT (cuando se presiona Ctrl+C)
// Esto cambia la variable server_running a 0, lo que permite salir del ciclo.

// Función para manejar la señal SIGINT
void handle_signal(int signal) {
    if (signal == SIGINT) {
        log_info(LOGGER_FILESYSTEM, "Servidor cerrándose...");
        server_running = 0;  // Cambiar el estado para salir del ciclo
    }
}

int main(int argc, char *argv[]) {

    signal(SIGINT, handle_signal);

    inicializar_config("filesystem");
    iniciar_archivos();

    //Iniciar servidor
    socket_filesystem = iniciar_servidor(PUERTO_ESCUCHA,LOGGER_FILESYSTEM,IP_FILESYSTEM,"FILESYSTEM");

    // CICLO PARA ESPERAR CONEXIONES Y CREAR HILOS POR CLIENTE
    while(server_running) {
        // ESPERAR A MEMORIA Y GESTIONAR LA CONEXION
        socket_filesystem_memoria = esperar_cliente(socket_filesystem, LOGGER_FILESYSTEM);

        // CREA UN HILO PARA MANEJAR CADA CLIENTE
        pthread_t thread_id;
        t_datos_cliente *datos_cliente = malloc(sizeof(t_datos_cliente));
        datos_cliente->socket_cliente = socket_filesystem_memoria;

        if(pthread_create(&thread_id, NULL, handle_client, (void *) datos_cliente) != 0) {
            log_error(LOGGER_FILESYSTEM, "Error al crear el hilo");
            free(datos_cliente);  // Liberar memoria en caso de error
        }
    
        // SEPARA AL HILO PARA QUE SE LIMPIE AUTOMATICAMENTE AL FINALIZAR
        pthread_detach(thread_id);
    
    }

    int sockets[] = {socket_filesystem, socket_filesystem_memoria, -1};
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

void iniciar_archivos() {
    char bitmap_path[256], bloques_path[256];
    sprintf(bitmap_path, "%s/bitmap.dat", MOUNT_DIR);
    sprintf(bloques_path, "%s/bloques.dat", MOUNT_DIR);

    FILE* bitmap = fopen(bitmap_path, "r+");
    if(!bitmap) { // VERIFICA SI ESTA VACIO
        bitmap = fopen(bitmap_path, "w");
        for (int i = 0; i < (BLOCK_COUNT + 7) / 8; i++) {
            fputc(0, bitmap); // VA PONIENDO 0 EN CADA BLOQUE DEL ARCHIVO (0 = BLOQUE LIBRE)
        }
        fclose(bitmap);
    }
    
    
    FILE* bloques = fopen(bloques_path, "r+");
    if (!bloques) { // VERIFICA SI ESTA VACIO
        bloques = fopen(bloques_path, "w");
        fseek(bloques, BLOCK_COUNT * BLOCK_SIZE - 1, SEEK_SET); 
        fputc('\0', bloques); // PONE UN 0 AL FINAL DEL ARCHIVO
        fclose(bloques);
    }
}

// FUNCION LA CUAL MANEJARA CADA PETICION DE UN HILO
void *handle_client(void *arg) {
    t_datos_cliente *datos_cliente = (t_datos_cliente *) arg;
    int socket_cliente = datos_cliente->socket_cliente;
    


    while (1) {
        int cod_op = recibir_operacion(socket_cliente);
        switch (cod_op) {
            case CREAR_ARCHIVO:
                //  char* nombre_archivo = recibir_nombre(socket_cliente);
                //  char* contenido = recibir_contenido(socket_cliente);
                //  int tamanio = recibir_tamanio(socket_cliente);
                //  int resultado = crear_archivo_dump(nombre_archivo, contenido, tamanio);
                //  if (resultado == -1) {
                //      enviar_respuesta_error(socket_cliente, "Error en la creación de archivo");
                //  }
                // log_info(LOGGER_FILESYSTEM, "## Fin de solicitud - Archivo: %s", nombre_archivo);
                log_info(LOGGER_FILESYSTEM, "OKY DOKY");
                break;
            case -1:
                //  log_error(LOGGER_FILESYSTEM, "El cliente se desconectó. Terminando hilo");
                //  close(socket_cliente);
                //  free(datos_cliente);
                log_info(LOGGER_FILESYSTEM, "OKEY");
                break;
            default:
                log_warning(LOGGER_FILESYSTEM, "Operacion desconocida");
                break;
        }
    }

    close(socket_cliente);
    free(datos_cliente);
    pthread_exit(NULL);
}