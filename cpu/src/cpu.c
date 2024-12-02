#include "include/gestor.h"

char* IP_MEMORIA;
char* PUERTO_MEMORIA;
char* PUERTO_ESCUCHA_DISPATCH;
char* PUERTO_ESCUCHA_INTERRUPT;
char* LOG_LEVEL;
char* IP_CPU;

t_log *LOGGER_CPU;
t_config *CONFIG_CPU;

int socket_cpu_dispatch_kernel;
int socket_cpu_interrupt_kernel;
int socket_cpu_memoria;

int main() {
    inicializar_config("cpu");
    log_info(LOGGER_CPU, "Iniciando CPU \n");

    int sockets[] = {socket_cpu_dispatch_kernel, socket_cpu_interrupt_kernel, socket_cpu_memoria, -1};
    iniciar_comunicaciones(sockets);

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

void inciar_comunicaciones(int socket[]) {
    // CONECTAR CON MEMORIA
    socket_cpu_memoria = crear_conexion(IP_MEMORIA, PUERTO_MEMORIA);
    if(socket_cpu_memoria == -1) {
        log_error(LOGGER_CPU, "Error al conectar con memoria");
        terminar_programa(CONFIG_CPU, LOGGER_CPU, sockets);
        exit(EXIT_FAILURE);
    }
    log_info(LOGGER_CPU, "Conexion con Memoria Establecida");

    // Crear hilo para manejar la comunicación con Memoria
    pthread_t hilo_memoria;
    t_procesar_conexion_args* args_memoria = malloc(sizeof(t_procesar_conexion_args));
    args_memoria->fd = socket_cpu_memoria;
    args_memoria->log = LOGGER_CPU;
    args_memoria->server_name = "Memoria";
    pthread_create(&hilo_memoria, NULL, procesar_conexion, (void*) args_memoria);
    pthread_detach(hilo_memoria);  

    // CONECTAR CON KERNEL, DISPATCH
    socket_cpu_dispatch_kernel = crear_conexion(IP_CPU, PUERTO_ESCUCHA_DISPATCH);
    if(socket_cpu_dispatch_kernel == -1) {
        log_error(LOGGER_CPU, "Error al conectar con Kernel Dispatch");
        terminar_programa(CONFIG_CPU, LOGGER_CPU, sockets);
        exit(EXIT_FAILURE);
    }
    log_info(LOGGER_CPU, "Conexion con Kernel Dispatch Establecida");

    // Crear hilo para manejar la comunicación con kernel_dispatch
    pthread_t hilo_kernel_dispatch;
    t_procesar_conexion_args* args_kernel_dispatch = malloc(sizeof(t_procesar_conexion_args));
    args_kernel_dispatch->fd = socket_cpu_dispatch_kernel;
    args_kernel_dispatch->log = LOGGER_CPU;
    args_kernel_dispatch->server_name = "Kernel Dispatch";
    pthread_create(&hilo_kernel_dispatch, NULL, procesar_conexion, (void*) args_kernel_dispatch);
    pthread_detach(hilo_kernel_dispatch);  

    // CONECTAR CON KERNEL, INTERRUPT
    socket_cpu_interrupt_kernel = crear_conexion(IP_CPU, PUERTO_ESCUCHA_INTERRUPT);
    if(socket_cpu_interrupt_kernel == -1) {
        log_error(LOGGER_CPU, "Error al conectar con Kernel Interrupt");
        terminar_programa(CONFIG_CPU, LOGGER_CPU, sockets);
        exit(EXIT_FAILURE);
    }
    log_info(LOGGER_CPU, "Conexion con Kernel Interrupt Establecida");

    // Crear hilo para manejar la comunicación con kernel_interrupt
    pthread_t hilo_kernel_interrupt;
    t_procesar_conexion_args* args_kernel_interrupt = malloc(sizeof(t_procesar_conexion_args));
    args_kernel_interrupt->fd = socket_cpu_interrupt_kernel;
    args_kernel_interrupt->log = LOGGER_CPU;
    args_kernel_interrupt->server_name = "Kernel Dispatch";
    pthread_create(&hilo_kernel_interrupt, NULL, procesar_conexion, (void*) args_kernel_interrupt);
    pthread_detach(hilo_kernel_interrupt);  
}

void* procesar_conexion(void* args) {
    t_procesar_conexion_args *args = (t_procesar_conexion_args *)void_args;
    t_log *logger = args->log;
    int socket = args->fd;
    char *server_name = args->server_name;
    free(args);

    while (socket != -1) {
        // Recibimos un paquete de memoria (respuesta)
        t_paquete* paquete = recibir_paquete_entero(socket);
        void* stream = paquete->buffer->stream;
        int size = paquete->buffer->size;

        switch (paquete->codigo_operacion) {
        // KERNEL LE MANDA A CPU EL HILO A EJECUTAR
        case HILO:
            hilo_actual = deserializar_paquete_tcb(stream, size);
            ejecutar_ciclo_instruccion();
            break;
        
        // KERNEL LE MANDA A CPU EL PROCESO DEL HILO EJECUTANDOSE
        case SOLICITUD_PROCESO:
            pcb_actual = deserializar_proceso(stream, size);
            break;

        // KERNEL LE MANDA UNA INTERRUPCION POR FINALIZACION DE QUANTUM A CPU
        case FINALIZACION_QUANTUM:
            hay_interrupcion = true;
            break;

        // MEMORIA LE RESPONDE OK SI SE PUDO ESCRIBIR EN MEMORIA
        case MENSAJE:
            char* respuesta = (char*)stream;
            if (strcmp(respuesta, "OK") == 0) {
                log_info(logger, "Se pudo escribir en memoria");
            } else {
                log_warning(logger, "Error al escribir en memoria.");
            }
            break;

        default:
            break;
        }
    }

    return NULL;
}