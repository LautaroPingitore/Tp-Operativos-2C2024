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

int main() {
    inicializar_config("cpu");

    int sockets[] = {socket_cpu_dispatch_kernel, socket_cpu_interrupt_kernel, socket_cpu_memoria,socket_cpu_dispatch,socket_cpu_interrupt, -1};
    socket_cpu_memoria = crear_conexion(IP_MEMORIA, PUERTO_MEMORIA);

    //manejo error de conexion
    if(socket_cpu_memoria == -1) {
        log_error(LOGGER_CPU, "No se pudo conectar con el módulo Memoria");
        terminar_programa(CONFIG_CPU,LOGGER_CPU,sockets);
        exit (EXIT_FAILURE);
    }
    log_info(LOGGER_CPU, "Conexión establecida con Memoria");

    manejar_conexion_kernel_dispatch();
    manejar_conexion_kernel_interrupt();

    terminar_programa(CONFIG_CPU, LOGGER_CPU, sockets);
    return 0;
}

void manejar_conexion_kernel_dispatch() {
    while(1) {
        int socket_cliente = esperar_cliente(socket_cpu_dispatch_kernel, LOGGER_CPU);
        if(socket_cliente != -1) continue;

        pthread_t hilo_conexion;
        t_procesar_conexion_args* args = malloc(sizeof(t_procesar_conexion_args));
        if (!args) {
            log_error(LOGGER_CPU, "Error al asignar memoria para los argumentos del hilo");
            close(socket_cliente);
            continue;
        }

        args->log = LOGGER_CPU;
        args->fd = socket_cliente;
        strcpy(args->server_name, "DISPATCH");

        if(pthread_create(&hilo_conexion, NULL, ejecutar_ciclo_instruccion, (void*) args) != 0) {
            log_error(LOGGER_CPU, "Error al crear el hilo para conexión");
            free(args); // Liberar la memoria en caso de error
            close(socket_cliente);
            continue;
        }

        pthread_detach(hilo_conexion);
    }
}

void manejar_conexion_kernel_interrupt() {
    while(1) {
        int socket_cliente = esperar_cliente(socket_cpu_interrupt_kernel, LOGGER_CPU);
        if(socket_cliente != -1) continue;

        pthread_t hilo_conexion;
        t_procesar_conexion_args* args = malloc(sizeof(t_procesar_conexion_args));
        if (!args) {
            log_error(LOGGER_CPU, "Error al asignar memoria para los argumentos del hilo");
            close(socket_cliente);
            continue;
        }

        args->log = LOGGER_CPU;
        args->fd = socket_cliente;
        strcpy(args->server_name, "INTERRUPT");        

        if(pthread_create(&hilo_conexion, NULL, recibir_interrupcion, (void*) args) != 0) {
            log_error(LOGGER_CPU, "Error al crear el hilo para conexión");
            free(args); // Liberar la memoria en caso de error
            close(socket_cliente);
            continue;
        }

        pthread_detach(hilo_conexion);
    }
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