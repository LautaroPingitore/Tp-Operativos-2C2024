#include <include/filesystem.h>

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
    BLOCK_SIZE = config_get_string_value(CONFIG_FILESYSTEM, "BLOCK_SIZE");
    BLOCK_COUNT = config_get_string_value(CONFIG_FILESYSTEM, "BLOCK_COUNT");
    RETARDO_ACCESO_BLOQUE = config_get_string_value(CONFIG_FILESYSTEM, "RETARDO_ACCESO_BLOQUE");
    LOG_LEVEL = config_get_string_value(CONFIG_FILESYSTEM, "LOG_LEVEL");

    IP_FILESYSTEM = config_get_string_value(CONFIG_FILESYSTEM,"IP_FILESYSTEM");
}

// int gestionarConexionConMemoria(){

// t_list* lista;
// while(1){
//     int cod_op = recibir_operacion(socket_filesystem_memoria);
//     switch(cod_op){
//     case MENSAJE:
//         recibir_mensaje(socket_filesystem_memoria, LOGGER_FILESYSTEM);
//         break;
//     case PAQUETE:
//         lista = recibir_paquete(socket_filesystem_memoria);
//         log_info(LOGGER_FILESYSTEM,"Me llegaron los siguientes valores:\n");
//         list_iterate(lista, (void*) iterator);
//         break;
//     case -1:
//         log_error(LOGGER_FILESYSTEM, "El cliente se desconecto. Terminando servidor");
//         return EXIT_FAILURE;
//     default:
//         log_warning(LOGGER_FILESYSTEM,"Operacion desconocida. No quieras meter la pata");
//         break;
//     }
// }
//     return EXIT_SUCCESS;
// }

// FUNCION LA CUAL MANEJARA CADA PETICION DE UN HILO
void *handle_client(void *arg) {
    t_datos_cliente *datos_cliente = (t_datos_cliente *) arg;
    int socket_cliente = datos_cliente->socket_cliente;
    
    t_list* lista;

    while (1) {
        int cod_op = recibir_operacion(socket_cliente);
        switch (cod_op) {
            case MENSAJE:
                recibir_mensaje(socket_cliente, LOGGER_FILESYSTEM);
                break;
            case PAQUETE:
                lista = recibir_paquete(socket_cliente);
                log_info(LOGGER_FILESYSTEM, "Me llegaron los siguientes valores:\n");
                list_iterate(lista, (void*) iterator);
                break;
            case -1:
                log_error(LOGGER_FILESYSTEM, "El cliente se desconecto. Terminando hilo");
                free(datos_cliente);
                pthread_exit(NULL);  // Terminar el hilo si el cliente se desconecta
            default:
                log_warning(LOGGER_FILESYSTEM, "Operacion desconocida. No quieras meter la pata");
                break;
        }
    }

    close(socket_cliente);
    free(datos_cliente);
    pthread_exit(NULL);
}


void iterator(char* value) {
	log_info(LOGGER_FILESYSTEM,"%s", value);
}