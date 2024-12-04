#include "include/gestor.h"

pthread_t hilo_server_kernel;
t_log* LOGGER_KERNEL;
t_config* CONFIG_KERNEL;

char* IP_MEMORIA;
char* PUERTO_MEMORIA;
char* IP_CPU;
char* PUERTO_CPU_DISPATCH;
char* PUERTO_CPU_INTERRUPT;
int QUANTUM;
char* LOG_LEVEL;
char* ALGORITMO_PLANIFICACION;

int socket_kernel_memoria;
int socket_kernel_cpu_dispatch;
int socket_kernel_cpu_interrupt;
lista_recursos recursos_globales;

pthread_t hilo_kernel_memoria;
pthread_t hilo_kernel_dispatch;
pthread_t hilo_kernel_interrupt;

int main(int argc, char* argv[]) {
    // VERIFICACIÓN DE QUE SE PASARON AL MENOS 3 ARGUMENTOS (programa, archivo pseudocódigo, tamaño proceso)
    if(argc < 3) {
        printf("Uso: %s [archivo_pseudocodigo] [tamanio_proceso]\n", argv[0]);
        return -1;
    }

    // OBTENCION DEL ARCHIVO DEL PSEUDOCODIGO Y EL TAMANIO DEL PROCESO
    char* archivo_pseudocodigo = argv[1];
    int tamanio_proceso = atoi(argv[2]);
    //lista_recursos recursos_globales;
    int sockets[] = {socket_kernel_memoria, socket_kernel_cpu_dispatch, socket_kernel_cpu_interrupt, -1};

    // INICIAR CONFIGURACION DE KERNEL
    inicializar_config("kernel");
    log_info(LOGGER_KERNEL, "Iniciando KERNEL \n");

    // INICIAR CONEXIONES
    iniciar_conexiones();

    // INICIAR LOS SEMAFOROS Y COLAS
    inicializar_kernel();

    // CREAR PROCESOS INICIAL Y LO EJECUTA
    crear_proceso(archivo_pseudocodigo, tamanio_proceso, 0);
    intentar_mover_a_execute();

    // NO SE SI HACE FALTA ESTE WHILE, YA QUE CUANDO HACES EL
    // INTENTAR MOVER EXECUTE, YA EJECUTAS ABSOLUTAMENTE TODO
    // while(list_size(tabla_procesos) > 0) {

    // }

    terminar_programa(CONFIG_KERNEL, LOGGER_KERNEL, sockets);

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

    LOGGER_KERNEL = iniciar_logger("kernel.log", "KERNEL");
    CONFIG_KERNEL = iniciar_config(config_path, "KERNEL");
    
    IP_MEMORIA = config_get_string_value(CONFIG_KERNEL, "IP_MEMORIA");
    PUERTO_MEMORIA = config_get_string_value(CONFIG_KERNEL, "PUERTO_MEMORIA");
    IP_CPU = config_get_string_value(CONFIG_KERNEL, "IP_CPU");
    PUERTO_CPU_DISPATCH = config_get_string_value(CONFIG_KERNEL, "PUERTO_CPU_DISPATCH");
    PUERTO_CPU_INTERRUPT = config_get_string_value(CONFIG_KERNEL, "PUERTO_CPU_INTERRUPT");
    ALGORITMO_PLANIFICACION = config_get_string_value(CONFIG_KERNEL, "ALGORITMO_PLANIFICACION");
    QUANTUM = config_get_int_value(CONFIG_KERNEL, "QUANTUM");
    LOG_LEVEL = config_get_string_value(CONFIG_KERNEL, "LOG_LEVEL");
}

void iniciar_conexiones() {
    // CONEXION CLIENTE MEMORIA
    socket_kernel_memoria = crear_conexion(IP_MEMORIA, PUERTO_MEMORIA);
    if (socket_kernel_memoria < 0) {
        log_error(LOGGER_KERNEL, "No se pudo conectar con el módulo FileSystem");
        exit(EXIT_FAILURE);
    }
    enviar_handshake("Hola Memoria, soy kernel", socket_kernel_memoria, HANDSHAKE_memoria);
    pthread_create(&hilo_kernel_memoria, NULL, escuchar_kernel_memoria, NULL);
    //pthread_detach(hilo_kernel_memoria);
    if (pthread_join(hilo_kernel_memoria, NULL) != 0) {
        log_error(LOGGER_KERNEL, "Error al esperar la finalización del hilo escuchar_kernel_mem.");
    } else {
        log_info(LOGGER_KERNEL, "El hilo escuchar_kernel_mem finalizó correctamente.");
    }
    
    // CONEXION CLIENTE CPU DISPATCH
    socket_kernel_cpu_dispatch = crear_conexion(IP_CPU, PUERTO_CPU_DISPATCH);
    if (socket_kernel_cpu_dispatch < 0) {
        log_error(LOGGER_KERNEL, "No se pudo conectar con el módulo CPU DISPATCH");
        exit(EXIT_FAILURE);
    }
    enviar_handshake("Hola Dispatch, soy Kernel", socket_kernel_cpu_dispatch, HANDSHAKE_dispatch);
    pthread_create(&hilo_kernel_dispatch, NULL, escuchar_kernel_cpu_dispatch, NULL);
    //pthread_detach(hilo_kernel_dispatch);
    if (pthread_join(hilo_kernel_dispatch, NULL) != 0) {
        log_error(LOGGER_KERNEL, "Error al esperar la finalización del hilo escuchar_kernel_dis.");
    } else {
        log_info(LOGGER_KERNEL, "El hilo escuchar_kernel_dis finalizó correctamente.");
    }

    socket_kernel_cpu_interrupt = crear_conexion(IP_CPU, PUERTO_CPU_INTERRUPT);
    if (socket_kernel_cpu_interrupt < 0) {
        log_error(LOGGER_KERNEL, "No se pudo conectar con el módulo CPU INTERRUPT");
        exit(EXIT_FAILURE);
    }
    enviar_handshake("Hola Interrupt, soy Kernel",socket_kernel_cpu_interrupt, HANDSHAKE_interrupt);
    pthread_create(&hilo_kernel_interrupt, NULL, escuchar_kernel_cpu_interrupt, NULL);
    //pthread_detach(hilo_kernel_interrupt);
    if (pthread_join(hilo_kernel_interrupt, NULL) != 0) {
        log_error(LOGGER_KERNEL, "Error al esperar la finalización del hilo escuchar_kernel_int.");
    } else {
        log_info(LOGGER_KERNEL, "El hilo escuchar_kernel_int finalizó correctamente.");
    }
}

void* escuchar_kernel_memoria() {
    while (1) {
        int cliente_socket = esperar_cliente(socket_kernel_memoria, LOGGER_KERNEL);
        if (cliente_socket > 0) {
            pthread_t hilo_cliente;
            t_procesar_conexion_args* args = malloc(sizeof(t_procesar_conexion_args));
            args->log = LOGGER_KERNEL;
            args->fd = cliente_socket;
            args->server_name = strdup("KERNEL");

            pthread_create(&hilo_cliente, NULL, procesar_conexion_memoria, (void*)args);
            pthread_detach(hilo_cliente);
        }
    }
    return NULL;
}

void* escuchar_kernel_cpu_dispatch() {
    while (1) {
        int cliente_socket = esperar_cliente(socket_kernel_cpu_dispatch, LOGGER_KERNEL);
        if (cliente_socket > 0) {
            pthread_t hilo_cliente;
            t_procesar_conexion_args* args = malloc(sizeof(t_procesar_conexion_args));
            args->log = LOGGER_KERNEL;
            args->fd = cliente_socket;
            args->server_name = strdup("KERNEL");

            pthread_create(&hilo_cliente, NULL, procesar_conexiones_cpu, (void*)args);
            pthread_detach(hilo_cliente);
        }
    }
    return NULL;
}

void* escuchar_kernel_cpu_interrupt() {
    while (1) {
        int cliente_socket = esperar_cliente(socket_kernel_cpu_interrupt, LOGGER_KERNEL);
        if (cliente_socket > 0) {
            pthread_t hilo_cliente;
            t_procesar_conexion_args* args = malloc(sizeof(t_procesar_conexion_args));
            args->log = LOGGER_KERNEL;
            args->fd = cliente_socket;
            args->server_name = strdup("KERNEL");

            pthread_create(&hilo_cliente, NULL, procesar_conexiones_cpu, (void*)args);
            pthread_detach(hilo_cliente);
        }
    }
    return NULL;
}


// OJO CON ESTA PORQUE ES EXTREMADAMENTE RARA
void* procesar_conexion_memoria(void* void_args) {
    t_procesar_conexion_args *args = (t_procesar_conexion_args *)void_args;
    t_log *logger = args->log;
    int socket = args->fd;
    //char* server_name = args->server_name;

    free(args);

    op_code cod;
    while (1) {
        // Recibir código de operación
        ssize_t bytes_recibidos = recv(socket, &cod, sizeof(op_code), 0);
        if (bytes_recibidos <= 0) { // El cliente cerró la conexión o hubo un error
            log_error(logger, "El cliente cerró la conexión.");
            break; // Salir del bucle y cerrar el hilo
        }

        switch (cod) {
        case HANDSHAKE_memoria:
            recibir_mensaje(socket, logger);
            break;

        case MENSAJE:
            char* respuesta = deserializar_mensaje(socket);
            if (strcmp(respuesta, "OK") == 0) {
                log_info(logger, "Proceso creado en memoria.");
            } else {
                log_warning(logger, "Error al crear proceso en memoria.");
            }
            break;
        
        default:
            log_error(logger, "Operacion desconocida");
            break;
        }
    }
    return NULL;
}

void* procesar_conexiones_cpu(void* void_args) {
    t_procesar_conexion_args *args = (t_procesar_conexion_args *)void_args;
    t_log *logger = args->log;
    int socket = args->fd;
    //char *server_name = args->server_name;
    free(args);

    op_code cod;
    while(1) {
        // Recibir código de operación
        ssize_t bytes_recibidos = recv(socket, &cod, sizeof(op_code), 0);
        if (bytes_recibidos <= 0) { // El cliente cerró la conexión o hubo un error 
            log_error(logger, "El cliente cerró la conexión.");
            break; // Salir del bucle y cerrar el hilo
        }

        switch (cod) {
        case HANDSHAKE_cpu:
            recibir_mensaje(socket, logger);
            break;
        
        case DEVOLVER_CONTROL_KERNEL:
            // Procesar el TCB recibido desde la CPU (cuando el CPU termina de ejecutar una instrucción)
            t_tcb* tcb = recibir_hilo(socket);
            if(tcb->motivo_desalojo == INTERRUPCION_BLOQUEO) {
                list_remove(cola_exec, 0);

                pthread_mutex_lock(&mutex_cola_ready);
                list_add(cola_ready, tcb);  // Agregar el TCB a la cola de ready
                pthread_mutex_unlock(&mutex_cola_ready);

                intentar_mover_a_execute();  // Intentar ejecutar el siguiente hilo
            }
            break;

        default:
            manejar_syscall(socket);  // Manejar syscalls para el kernel
            break;
        }

    }

    return NULL;
}
