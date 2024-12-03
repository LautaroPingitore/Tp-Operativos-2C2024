#include "include/gestor.h"

char* IP_MEMORIA;
char* PUERTO_MEMORIA;
char* PUERTO_ESCUCHA_DISPATCH;
char* PUERTO_ESCUCHA_INTERRUPT;
char* LOG_LEVEL;
char* IP_CPU;

t_log *LOGGER_CPU;
t_config *CONFIG_CPU;

int socket_cpu_dispatch;
int socket_cpu_interrupt;
int socket_cpu_dispatch_kernel;
int socket_cpu_interrupt_kernel;
int socket_cpu_memoria;

pthread_t hilo_servidor_interrupt;

int main() {
    inicializar_config("cpu");
    log_info(LOGGER_CPU, "Iniciando CPU \n");

    iniciar_conexiones();

    int sockets[] = {socket_cpu_dispatch_kernel, socket_cpu_interrupt_kernel, socket_cpu_memoria, socket_cpu_dispatch, socket_cpu_interrupt, -1};
    terminar_programa(CONFIG_CPU, LOGGER_CPU, sockets);
    return 0;
}

void inicializar_config(char* arg){
    
    /*
    char config_path[256];
    strcpy(config_path, arg);
    strcat(config_path, ".config");
    */

    char config_path[256];
    strcpy(config_path, "../");
    strcat(config_path, arg);
    strcat(config_path, ".config");


    LOGGER_CPU = iniciar_logger("cpu.log", "CPU");
    CONFIG_CPU = iniciar_config(config_path, "CPU");
    
    IP_MEMORIA = config_get_string_value(CONFIG_CPU, "IP_MEMORIA");
    PUERTO_MEMORIA = config_get_string_value(CONFIG_CPU, "PUERTO_MEMORIA");
    PUERTO_ESCUCHA_DISPATCH = config_get_string_value(CONFIG_CPU, "PUERTO_ESCUCHA_DISPATCH");
    PUERTO_ESCUCHA_INTERRUPT = config_get_string_value(CONFIG_CPU, "PUERTO_ESCUCHA_INTERRUPT");
    LOG_LEVEL = config_get_string_value(CONFIG_CPU, "LOG_LEVEL");

    IP_CPU = config_get_string_value(CONFIG_CPU,"IP_CPU");
}

void iniciar_conexiones() {
    // SERVER CPU DISPATCH
    socket_cpu_dispatch = iniciar_servidor(PUERTO_ESCUCHA_DISPATCH, LOGGER_CPU, IP_CPU, "CPU_DISPATCH");

    // SERVER CPU INTERRUPT
    socket_cpu_interrupt = iniciar_servidor(PUERTO_ESCUCHA_INTERRUPT, LOGGER_CPU, IP_CPU, "CPU_INTERRUPT");

    // CONEXION CLIENTE CON MEMORIA
    socket_cpu_memoria = crear_conexion(IP_MEMORIA, PUERTO_MEMORIA);

    // HILOS SERVIDOR
    pthread_create(&hilo_servidor_interrupt, NULL, escuchar_cpu, NULL);
    if (pthread_join(hilo_servidor_interrupt, NULL) != 0) {
        log_error(LOGGER_CPU, "Error al esperar la finalización del hilo escuchar_cpu.");
    } else {
        log_info(LOGGER_CPU, "El hilo escuchar_cpu finalizó correctamente.");
    }

}

void* escuchar_cpu() {
    log_info(LOGGER_CPU, "El hilo de escuchar_cpu ha iniciado.");
    while (server_escuchar(LOGGER_CPU, "CPU_INTERRUPT", socket_cpu_interrupt)) {
        log_info(LOGGER_CPU, "Conexión procesada.");
    }
    log_warning(LOGGER_CPU, "El servidor de CPU_INTERRUPT terminó inesperadamente.");
    return NULL;
}

int server_escuchar(t_log *logger, char *server_name, int server_socket) {
    int cliente_socket = esperar_cliente(server_socket, logger);

	if (cliente_socket != -1)
	{
		pthread_t hilo;
		t_procesar_conexion_args *args = malloc(sizeof(t_procesar_conexion_args));
		args->log = logger;
		args->fd = cliente_socket;
		args->server_name = server_name;
		if (strcmp(server_name, "CPU_DISPATCH") == 0)
		{
			pthread_create(&hilo, NULL, (void *)procesar_conexion_dispatch, args);
		}
		else if (strcmp(server_name, "CPU_INTERRUPT") == 0)
		{
			pthread_create(&hilo, NULL, (void *)procesar_conexion_interrupt, args);
		}
		return 1;
	}
	return 0;
}

void* procesar_conexion_dispatch(void* void_args) {
    t_procesar_conexion_args *args = (t_procesar_conexion_args *)void_args;
    t_log *logger = args->log;
    int socket = args->fd;

    free(args);

    op_code cod;
    while(1) {
        // Recibir código de operación
        ssize_t bytes_recibidos = recv(socket, &cod, sizeof(op_code), 0);
        if (bytes_recibidos <= 0) { // El cliente cerró la conexión o hubo un error
            log_error(logger, "El cliente cerró la conexión.");
            break; // Salir del bucle y cerrar el hilo
        }

        t_paquete* paquete = recibir_paquete(socket);
        void* stream = paquete->buffer->stream;
        int size = paquete->buffer->size;

        switch(cod) {
            case HANDSHAKE_kernel:
                recibir_mensaje(socket, logger);
                break;

            case HILO:
                hilo_actual = deserializar_paquete_tcb(stream, size);
                ejecutar_ciclo_instruccion();
                break;

            case PROCESO:
                pcb_actual = deserializar_proceso(stream, size);
                break;

            case MENSAJE:
                char* respuesta = (char*)stream;
                if (strcmp(respuesta, "OK") == 0) {
                    log_info(logger, "Se pudo escribir en memoria");
                } else {
                    log_warning(logger, "Error al escribir en memoria.");
                }
                break;

            default:
                log_error(logger, "Codigo de operacion desconocida: %d", cod);
                break;
        }

        eliminar_paquete(paquete);
    }

    close(socket);
    return NULL;
}

void* procesar_conexion_interrupt(void* void_args) {
    t_procesar_conexion_args *args = (t_procesar_conexion_args *)void_args;
    t_log *logger = args->log;
    int socket = args->fd;

    free(args);

    op_code cod;
    while(1) {
        // Recibir código de operación
        ssize_t bytes_recibidos = recv(socket, &cod, sizeof(op_code), 0);
        if (bytes_recibidos <= 0) { // El cliente cerró la conexión o hubo un error
            log_error(logger, "El cliente cerró la conexión.");
            break; // Salir del bucle y cerrar el hilo
        }

        t_paquete* paquete = recibir_paquete(socket);

        switch (cod) {
            case FINALIZACION_QUANTUM:
                hay_interrupcion = true;
                break;
            
            default:
                log_error(logger, "Codigo de operacion desconocida: %d", cod);
                break;
        }

        eliminar_paquete(paquete);

    }

    close(socket);
    return NULL;
}
