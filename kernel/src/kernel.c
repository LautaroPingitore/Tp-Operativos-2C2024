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

// EL ARHCIVO DE PSEUDOCODIGO ESTA EN UNA CARPETA DE HOME LLAMADA scripts-pruebas
int main(int argc, char* argv[]) {
    // VERIFICACIÓN DE QUE SE PASARON AL MENOS 3 ARGUMENTOS (programa, archivo pseudocódigo, tamaño proceso)
    if(argc != 4) {
        printf("Uso: %s [archivo_config] [archivo_pseudocodigo] [tamanio_proceso]\n", argv[0]);
        return -1;
    }

    // OBTENCION DEL ARCHIVO DEL PSEUDOCODIGO Y EL TAMANIO DEL PROCESO
    char* config = argv[1];
    char pseudo_path[256];
    strcpy(pseudo_path, "/home/utnso/scripts-pruebas/");
    strcat(pseudo_path, argv[2]);

    int tamanio_proceso = atoi(argv[3]);
    //lista_recursos recursos_globales;

    // INICIAR CONFIGURACION DE KERNEL
    inicializar_config(config);
    log_info(LOGGER_KERNEL, "Iniciando KERNEL \n");

    // INICIAR CONEXIONES
    // Iniciar conexiones
    if (!iniciar_conexiones()) {
        log_error(LOGGER_KERNEL, "No se pudieron establecer las conexiones necesarias.");
        return -1;
    }

    // INICIAR LOS SEMAFOROS Y COLAS
    inicializar_kernel();

    // CREAR PROCESOS INICIAL Y LO EJECUTA
    crear_proceso(pseudo_path, tamanio_proceso, 0);
    intentar_mover_a_execute();

    // Manejar comunicaciones (cada una en su propio flujo)
    manejar_comunicaciones_memoria();
    manejar_comunicaciones_cpu_dispatch();
    manejar_comunicaciones_cpu_interrupt();

    int sockets[] = {socket_kernel_memoria, socket_kernel_cpu_dispatch, socket_kernel_cpu_interrupt, -1};
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
    strcpy(config_path, "../configs/");
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

bool iniciar_conexiones() {
    // Conectar a Memoria
    socket_kernel_memoria = crear_conexion(IP_MEMORIA, PUERTO_MEMORIA);
    if (socket_kernel_memoria < 0) {
        log_error(LOGGER_KERNEL, "No se pudo conectar con el módulo Memoria");
        return false;
    }
    enviar_handshake(socket_kernel_memoria, HANDSHAKE_kernel);

    // Conectar a CPU Dispatch
    socket_kernel_cpu_dispatch = crear_conexion(IP_CPU, PUERTO_CPU_DISPATCH);
    if (socket_kernel_cpu_dispatch < 0) {
        log_error(LOGGER_KERNEL, "No se pudo conectar con el módulo CPU Dispatch");
        return false;
    }
    enviar_handshake(socket_kernel_cpu_dispatch, HANDSHAKE_dispatch);

    // Conectar a CPU Interrupt
    socket_kernel_cpu_interrupt = crear_conexion(IP_CPU, PUERTO_CPU_INTERRUPT);
    if (socket_kernel_cpu_interrupt < 0) {
        log_error(LOGGER_KERNEL, "No se pudo conectar con el módulo CPU Interrupt");
        return false;
    }
    enviar_handshake(socket_kernel_cpu_interrupt, HANDSHAKE_interrupt);

    log_info(LOGGER_KERNEL, "Todas las conexiones establecidas correctamente.");
    return true;
}

// Función genérica para manejar comunicaciones
void manejar_comunicaciones(int socket, const char* nombre_modulo) {
    op_code cod;
    while (1) {
        ssize_t bytes_recibidos = recv(socket, &cod, sizeof(op_code), MSG_WAITALL);
        if (bytes_recibidos <= 0) {
            if (bytes_recibidos == 0) {
                log_warning(LOGGER_KERNEL, "Socket cerrado por el módulo %s.", nombre_modulo);
            } else {
                log_error(LOGGER_KERNEL, "Error al recibir datos desde el módulo %s.", nombre_modulo);
            }
            break;
        }

        log_info(LOGGER_KERNEL, "Código de operación recibido de %s: %d", nombre_modulo, cod);
        switch (cod) {
            case HANDSHAKE_memoria:
            case HANDSHAKE_dispatch:
            case HANDSHAKE_interrupt:
                log_info(LOGGER_KERNEL, "Handshake recibido de %s.", nombre_modulo);
                break;

            case MENSAJE: {
                char* respuesta = deserializar_mensaje(socket);
                if (strcmp(respuesta, "OK") == 0) {
                    log_info(LOGGER_KERNEL, "Mensaje OK recibido de %s.", nombre_modulo);
                } else {
                    log_warning(LOGGER_KERNEL, "Mensaje de error recibido de %s.", nombre_modulo);
                }
                free(respuesta);
                break;
            }

            case DEVOLVER_CONTROL_KERNEL: {
                if (strcmp(nombre_modulo, "CPU_INTERRUPT") == 0) {
                    t_tcb* tcb = recibir_hilo(socket);
                    if (tcb->motivo_desalojo == INTERRUPCION_BLOQUEO) {
                        list_remove(cola_exec, 0);

                        pthread_mutex_lock(&mutex_cola_ready);
                        list_add(cola_ready, tcb);
                        pthread_mutex_unlock(&mutex_cola_ready);

                        intentar_mover_a_execute();
                    }
                }
                break;
            }

            default:
                if(strcmp(nombre_modulo, "CPU_DISPATCH") == 0) {
                    manejar_syscall(socket_kernel_cpu_dispatch);
                } else {
                    log_warning(LOGGER_KERNEL, "Operación desconocida recibida desde %s.", nombre_modulo);
                }
                break;
        }
    }
    log_info(LOGGER_KERNEL, "Finalizando conexión con el módulo %s.", nombre_modulo);
    close(socket);
}

// Llamadas específicas para cada módulo
void manejar_comunicaciones_memoria() {
    manejar_comunicaciones(socket_kernel_memoria, "MEMORIA");
}

void manejar_comunicaciones_cpu_dispatch() {
    manejar_comunicaciones(socket_kernel_cpu_dispatch, "CPU_DISPATCH");
}

void manejar_comunicaciones_cpu_interrupt() {
    manejar_comunicaciones(socket_kernel_cpu_interrupt, "CPU_INTERRUPT");
}




















































































/*
void iniciar_conexiones() {
    // Conexión cliente Memoria
    socket_kernel_memoria = crear_conexion(IP_MEMORIA, PUERTO_MEMORIA);
    if (socket_kernel_memoria < 0) {
        log_error(LOGGER_KERNEL, "No se pudo conectar con el módulo Memoria");
        exit(EXIT_FAILURE);
    }
    enviar_handshake(socket_kernel_memoria, HANDSHAKE_kernel);
    if(pthread_create(&hilo_kernel_memoria, NULL, escuchar_kernel_memoria, NULL) != 0){
        log_error(LOGGER_KERNEL, "ERROR CREANDO HILO 1");
    }
    pthread_detach(hilo_kernel_memoria);

    // Conexión cliente CPU Dispatch
    socket_kernel_cpu_dispatch = crear_conexion(IP_CPU, PUERTO_CPU_DISPATCH);
    if (socket_kernel_cpu_dispatch < 0) {
        log_error(LOGGER_KERNEL, "No se pudo conectar con el módulo CPU Dispatch");
        exit(EXIT_FAILURE);
    }
    enviar_handshake(socket_kernel_cpu_dispatch, HANDSHAKE_dispatch);
    if(pthread_create(&hilo_kernel_dispatch, NULL, escuchar_kernel_cpu_dispatch, NULL) != 0){
        log_error(LOGGER_KERNEL, "ERROR CREANDO HILO 2");
    }
    pthread_detach(hilo_kernel_dispatch);

    // Conexión cliente CPU Interrupt
    socket_kernel_cpu_interrupt = crear_conexion(IP_CPU, PUERTO_CPU_INTERRUPT);
    if (socket_kernel_cpu_interrupt < 0) {
        log_error(LOGGER_KERNEL, "No se pudo conectar con el módulo CPU Interrupt");
        exit(EXIT_FAILURE);
    }
    enviar_handshake(socket_kernel_cpu_interrupt, HANDSHAKE_interrupt);
    if(pthread_create(&hilo_kernel_interrupt, NULL, escuchar_kernel_cpu_interrupt, NULL) != 0){
        log_error(LOGGER_KERNEL, "ERROR CREANDO HILO 3");
    }
    pthread_detach(hilo_kernel_interrupt);

    log_warning(LOGGER_KERNEL, "FINALIZO INICIAR CONEXIONES XD");
}

int server_escuchar(char *server_name, int server_socket) {

    // log_warning(LOGGER_KERNEL, "CHECKPOINT AL PRINCIPIO DE SERVER ESCUCHAR");
    while (1) {
        //og_info(LOGGER_KERNEL, "[%s] Esperando conexión de cliente...", server_name);

        int cliente_socket = esperar_cliente(server_socket, LOGGER_KERNEL);

        if (cliente_socket == -1) {
            //log_warning(LOGGER_KERNEL, "[%s] Error al aceptar conexión del cliente. Reintentando...", server_name);
            break;
        }

        //log_info(LOGGER_KERNEL, "[%s] Cliente conectado. Procesando conexión...", server_name);

        pthread_t hilo_cliente;
        t_procesar_conexion_args *args = malloc(sizeof(t_procesar_conexion_args));
        if (!args) {
            //log_error(LOGGER_KERNEL, "[%s] Error al asignar memoria para las conexiones.", server_name);
            close(cliente_socket);
            break;
        }

        args->log = LOGGER_KERNEL;
        args->fd = cliente_socket;
        args->server_name = strdup(server_name);

        if (pthread_create(&hilo_cliente, NULL, procesar_conexiones, (void *)args) != 0) {
            //log_error(LOGGER_KERNEL, "[%s] Error al crear hilo para cliente.", server_name);
            
            free(args);
            close(cliente_socket);
            break;
        }

        // Separar el hilo del flujo principal
        pthread_detach(hilo_cliente);

        //log_info(LOGGER_KERNEL, "[%s] Hilo creado para procesar la conexión.", server_name);
    }

    // Este punto no se alcanzará debido al ciclo infinito.
    //
    EL, "[%s] Server terminado inesperadamente.", server_name);
    return -1; // Para mantener compatibilidad, aunque no debería retornar.
}

void* escuchar_kernel_memoria() {
    log_info(LOGGER_KERNEL, "Hilo de escucha para Memoria iniciado.");
    while (server_escuchar("KERNEL-MEMORIA", socket_kernel_memoria) != -1) {
        //log_info(LOGGER_KERNEL, "Conexión procesada.");
    }
    log_warning(LOGGER_KERNEL, "El servidor para Kernel terminó inesperadamente.");
    return NULL;
}

void* escuchar_kernel_cpu_dispatch() {
    log_info(LOGGER_KERNEL, "Hilo de escucha para CPU Dispatch iniciado.");
    while (server_escuchar("KERNEL-CPU_DISPATCH", socket_kernel_cpu_dispatch != -1)) {
        //log_info(LOGGER_KERNEL, "Conexión procesada.");
    }
    log_warning(LOGGER_KERNEL, "El servidor para CPU Dispatch terminó inesperadamente.");
    return NULL;
}

void* 
() {
    log_info(LOGGER_KERNEL, "Hilo de escucha para CPU Interrupt iniciado.");
    while (server_escuchar("KERNEL-CPU_INTERRUPT", socket_kernel_cpu_interrupt != -1)) {
        //log_info(LOGGER_KERNEL, "Conexión procesada.");
    }
    log_warning(LOGGER_KERNEL, "El servidor para CPU Interrupt terminó inesperadamente.");
    return NULL;
}

void* procesar_conexiones(void* void_args) {
    t_procesar_conexion_args *args = (t_procesar_conexion_args *)void_args;
    //t_log *logger = args->log;
    int socket = args->fd;
    char* server_name = args->server_name;

    free(args);

    op_code cod;
    while(1) {    
        ssize_t bytes_recibidos = recv(socket, &cod, sizeof(op_code), MSG_WAITALL);
        if (bytes_recibidos <= 0) {
            if (bytes_recibidos == 0) {
                log_warning(LOGGER_KERNEL, "Socket cerrado por el cliente.");
            } else {
                log_error(LOGGER_KERNEL, "Error al recibir datos.");
            }
            break;
        }

        log_info(LOGGER_KERNEL, "Código de operación recibido: %d", cod);        
        switch (cod) {
            case HANDSHAKE_memoria:
                log_info(LOGGER_KERNEL, "## MEMORIA Conectado - FD del socket: <%d>", socket);
                break;

            case MENSAJE:
                char* respuesta = deserializar_mensaje(socket);
                if (strcmp(respuesta, "OK") == 0) {
                    log_info(LOGGER_KERNEL, "Proceso creado en memoria.");
                } else {
                    log_warning(LOGGER_KERNEL, "Error al crear proceso en memoria.");
                }
                break;
            
            case HANDSHAKE_cpu:
                log_info(LOGGER_KERNEL, "## CPU Conectado - FD del socket: <%d>", socket);
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
                    log_error(LOGGER_KERNEL, "Operacion desconocida");
                }
                break;
        }
    }

    log_warning(LOGGER_KERNEL, "Finalizando conexión con el cliente.");
    close(socket); // Cerrar el socket del cliente
    free(args->server_name);
    return NULL;
}
*/