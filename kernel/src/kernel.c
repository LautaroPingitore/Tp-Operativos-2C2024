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
    pthread_exit(NULL); // Evita que el hilo principal finalice y permite que los hilos creados sigan ejecutándose
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
    // Conexión cliente Memoria
    socket_kernel_memoria = crear_conexion(IP_MEMORIA, PUERTO_MEMORIA);
    if (socket_kernel_memoria < 0) {
        log_error(LOGGER_KERNEL, "No se pudo conectar con el módulo Memoria");
        exit(EXIT_FAILURE);
    }
    enviar_handshake(socket_kernel_memoria, HANDSHAKE_memoria);
    pthread_create(&hilo_kernel_memoria, NULL, escuchar_kernel_memoria, NULL);
    pthread_detach(hilo_kernel_memoria);

    // Conexión cliente CPU Dispatch
    socket_kernel_cpu_dispatch = crear_conexion(IP_CPU, PUERTO_CPU_DISPATCH);
    if (socket_kernel_cpu_dispatch < 0) {
        log_error(LOGGER_KERNEL, "No se pudo conectar con el módulo CPU Dispatch");
        exit(EXIT_FAILURE);
    }
    enviar_handshake(socket_kernel_cpu_dispatch, HANDSHAKE_dispatch);
    pthread_create(&hilo_kernel_dispatch, NULL, escuchar_kernel_cpu_dispatch, NULL);
    pthread_detach(hilo_kernel_dispatch);

    // Conexión cliente CPU Interrupt
    socket_kernel_cpu_interrupt = crear_conexion(IP_CPU, PUERTO_CPU_INTERRUPT);
    if (socket_kernel_cpu_interrupt < 0) {
        log_error(LOGGER_KERNEL, "No se pudo conectar con el módulo CPU Interrupt");
        exit(EXIT_FAILURE);
    }
    enviar_handshake(socket_kernel_cpu_interrupt, HANDSHAKE_interrupt);
    pthread_create(&hilo_kernel_interrupt, NULL, escuchar_kernel_cpu_interrupt, NULL);
    pthread_detach(hilo_kernel_interrupt);
}

void* escuchar_kernel_memoria() {
    log_info(LOGGER_KERNEL, "Hilo de escucha para Memoria iniciado.");
    while (server_escuchar(LOGGER_KERNEL, "KERNEL-MEMORIA", socket_kernel_memoria)) {
        log_info(LOGGER_KERNEL, "Conexión procesada.");
    }
    log_warning(LOGGER_KERNEL, "El servidor para Memoria terminó inesperadamente.");
    return NULL;
}

int server_escuchar(t_log *logger, char *server_name, int server_socket) {
    while (1) {
        log_info(logger, "[%s] Esperando conexión de cliente...", server_name);

        int cliente_socket = esperar_cliente(server_socket, logger);

        if (cliente_socket == -1) {
            log_warning(logger, "[%s] Error al aceptar conexión del cliente. Reintentando...", server_name);
            continue;
        }

        log_info(logger, "[%s] Cliente conectado. Procesando conexión...", server_name);

        pthread_t hilo_cliente;
        t_procesar_conexion_args *args = malloc(sizeof(t_procesar_conexion_args));
        if (!args) {
            log_error(logger, "[%s] Error al asignar memoria para las conexiones.", server_name);
            close(cliente_socket);
            continue;
        }

        args->log = logger;
        args->fd = cliente_socket;
        args->server_name = strdup(server_name);

        if (pthread_create(&hilo_cliente, NULL, procesar_conexiones, (void *)args) != 0) {
            log_error(logger, "[%s] Error al crear hilo para cliente.", server_name);
            free(args->server_name);
            free(args);
            close(cliente_socket);
            continue;
        }

        // Separar el hilo del flujo principal
        pthread_detach(hilo_cliente);

        log_info(logger, "[%s] Hilo creado para procesar la conexión.", server_name);
    }

    // Este punto no se alcanzará debido al ciclo infinito.
    log_warning(logger, "[%s] Server terminado inesperadamente.", server_name);
    return -1; // Para mantener compatibilidad, aunque no debería retornar.
}


void* escuchar_kernel_cpu_dispatch() {
    log_info(LOGGER_KERNEL, "Hilo de escucha para CPU Dispatch iniciado.");
    while (server_escuchar(LOGGER_KERNEL, "KERNEL-CPU_DISPATCH", socket_kernel_cpu_dispatch)) {
        log_info(LOGGER_KERNEL, "Conexión procesada.");
    }
    log_warning(LOGGER_KERNEL, "El servidor para CPU Dispatch terminó inesperadamente.");
    return NULL;
}

void* escuchar_kernel_cpu_interrupt() {
    log_info(LOGGER_KERNEL, "Hilo de escucha para CPU Interrupt iniciado.");
    while (server_escuchar(LOGGER_KERNEL, "KERNEL-CPU_INTERRUPT", socket_kernel_cpu_interrupt)) {
        log_info(LOGGER_KERNEL, "Conexión procesada.");
    }
    log_warning(LOGGER_KERNEL, "El servidor para CPU Interrupt terminó inesperadamente.");
    return NULL;
}

void* procesar_conexiones(void* void_args) {
    t_procesar_conexion_args *args = (t_procesar_conexion_args *)void_args;
    t_log *logger = args->log;
    int socket = args->fd;
    char* server_name = args->server_name;

    free(args);

    op_code cod;
    while(1) {
        ssize_t bytes_recibidos = recv(socket, &cod, sizeof(op_code), 0);
        if (bytes_recibidos <= 0) { // El cliente cerró la conexión o hubo un error
            log_error(logger, "El cliente cerró la conexión.");
            break; // Salir del bucle y cerrar el hilo
        }
        
        switch (cod) {
            case HANDSHAKE_memoria:
                log_info(logger, "## %s Conectado - FD del socket: <%d>", server_name, socket);
                break;

            case MENSAJE:
                char* respuesta = deserializar_mensaje(socket);
                if (strcmp(respuesta, "OK") == 0) {
                    log_info(logger, "Proceso creado en memoria.");
                } else {
                    log_warning(logger, "Error al crear proceso en memoria.");
                }
                break;
            
            case HANDSHAKE_cpu:
                log_info(logger, "## %s Conectado - FD del socket: <%d>", server_name, socket);
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
                if(strcmp(server_name, "CPU_DISPATCH") == 0) {
                    manejar_syscall(socket);  // Manejar syscalls para el kernel
                } else {
                    log_error(logger, "Operacion desconocida");
                }
                break;

        }
    }

    log_warning(logger, "El cliente se desconectó de %s.", server_name);
    close(socket);
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
