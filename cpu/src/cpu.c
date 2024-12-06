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

pthread_t hilo_servidor_dispatch;
pthread_t hilo_servidor_interrupt;

uint32_t base_pedida = 0;
uint32_t limite_pedido = 0;
uint32_t valor_memoria = 0;
t_instruccion* instruccion_actual = NULL;

sem_t sem_base, sem_limite, sem_valor_memoria, sem_instruccion;
sem_t sem_mutex_globales;

int main() {
    inicializar_config("../configs/cpu");
    log_info(LOGGER_CPU, "Iniciando CPU");

    iniciar_semaforos();
    iniciar_conexiones();

    pthread_exit(NULL); // Evita que el hilo principal finalice y permite que los hilos creados sigan ejecutándose
    int sockets[] = {socket_cpu_dispatch_kernel, socket_cpu_interrupt_kernel, socket_cpu_memoria, -1};
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
    strcpy(config_path, "../configs/");
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
    // Servidor CPU Dispatch
    socket_cpu_dispatch_kernel = iniciar_servidor(PUERTO_ESCUCHA_DISPATCH, LOGGER_CPU, IP_CPU, "CPU_DISPATCH");
    if (socket_cpu_dispatch_kernel == -1) {
        log_error(LOGGER_CPU, "No se pudo iniciar el servidor CPU_DISPATCH.");
        exit(EXIT_FAILURE);
    }

    // Servidor CPU Interrupt
    socket_cpu_interrupt_kernel = iniciar_servidor(PUERTO_ESCUCHA_INTERRUPT, LOGGER_CPU, IP_CPU, "CPU_INTERRUPT");
    if (socket_cpu_interrupt_kernel == -1) {
        log_error(LOGGER_CPU, "No se pudo iniciar el servidor CPU_INTERRUPT.");
        exit(EXIT_FAILURE);
    }

    // Conexión con Memoria
    socket_cpu_memoria = crear_conexion(IP_MEMORIA, PUERTO_MEMORIA);
    if (socket_cpu_memoria == -1) {
        log_error(LOGGER_CPU, "No se pudo conectar con el módulo Memoria.");
        exit(EXIT_FAILURE);
    }

    // Iniciar hilos de escucha
    pthread_create(&hilo_servidor_dispatch, NULL, escuchar_cpu_dispatch, NULL);
    pthread_detach(hilo_servidor_dispatch);

    pthread_create(&hilo_servidor_interrupt, NULL, escuchar_cpu_interrupt, NULL);
    pthread_detach(hilo_servidor_interrupt);

}

// void* escuchar_cpu() {
//     log_info(LOGGER_CPU, "El hilo de escuchar_cpu ha iniciado.");
//     while (server_escuchar("CPU_INTERRUPT", socket_cpu_interrupt)) {
//         log_info(LOGGER_CPU, "Conexión procesada.");
//     }
//     log_warning(LOGGER_CPU, "El servidor de CPU_INTERRUPT terminó inesperadamente.");
//     return NULL;
// }

void* escuchar_cpu_dispatch() {
    log_info(LOGGER_CPU, "El hilo de escucha para CPU_DISPATCH ha iniciado.");
    while (server_escuchar("CPU_DISPATCH", socket_cpu_dispatch_kernel)) {
        log_info(LOGGER_CPU, "Conexión procesada en CPU_DISPATCH.");
    }
    log_warning(LOGGER_CPU, "El servidor de CPU_DISPATCH terminó inesperadamente.");
    return NULL;
}

void* escuchar_cpu_interrupt() {
    log_info(LOGGER_CPU, "El hilo de escucha para CPU_INTERRUPT ha iniciado.");
    while (server_escuchar("CPU_INTERRUPT", socket_cpu_interrupt_kernel)) {
        log_info(LOGGER_CPU, "Conexión procesada en CPU_INTERRUPT.");
    }
    log_warning(LOGGER_CPU, "El servidor de CPU_INTERRUPT terminó inesperadamente.");
    return NULL;
}

int server_escuchar(char *server_name, int server_socket) {
    while (1) {
        int cliente_socket = esperar_cliente(server_socket, LOGGER_CPU);
        if (cliente_socket == -1) {
            log_warning(LOGGER_CPU, "[%s] Error al aceptar conexión. Reintentando...", server_name);
            continue;
        }

        pthread_t hilo_cliente;
        t_procesar_conexion_args *args = malloc(sizeof(t_procesar_conexion_args));
        if (!args) {
            log_error(LOGGER_CPU, "[%s] Error al asignar memoria para las conexiones.", server_name);
            close(cliente_socket);
            continue;
        }

        args->log = LOGGER_CPU;
        args->fd = cliente_socket;
        args->server_name = strdup(server_name);

        if (strcmp(server_name, "CPU_DISPATCH") == 0) {
            pthread_create(&hilo_cliente, NULL, procesar_conexion_dispatch, (void*) args);
        } else if (strcmp(server_name, "CPU_INTERRUPT") == 0) {
            pthread_create(&hilo_cliente, NULL, procesar_conexion_interrupt, (void*) args);
        }

        pthread_detach(hilo_cliente);
    }

    return 0; // Este punto no se alcanza debido al ciclo infinito.
}

void* procesar_conexion_dispatch(void* void_args) {
    t_procesar_conexion_args *args = (t_procesar_conexion_args *)void_args;
    //t_log *logger = args->log;
    int socket = args->fd;
    //char* server_name = args->server_name;
    free(args);

    op_code cod;
    while(1) {
        if(socket < 0) {
            log_error(LOGGER_CPU, "Socket inválido: %d", socket);
            break;
        }
        // Recibir código de operación
        ssize_t bytes_recibidos = recv(socket, &cod, sizeof(op_code), MSG_WAITALL);
        log_warning(LOGGER_CPU, "Me llego el codigo nro %d", cod);
        // if (bytes_recibidos != sizeof(op_code)) {
        //     log_error(LOGGER_CPU, "Error al recibir código de operación, bytes recibidos: %zd", bytes_recibidos);
        //     break;
        // }
        if (bytes_recibidos == -1) {
            log_error(LOGGER_CPU, "Error en recv");
        } else if (bytes_recibidos == 0) {
            log_warning(LOGGER_CPU, "Socket cerrado por el cliente");
        } else {
            log_info(LOGGER_CPU, "Se recibió el código de operación: %d", cod);
        }

        switch(cod) {
            case HANDSHAKE_dispatch:
                log_info(LOGGER_CPU, "## KERNEL_DIPATCH Conectado - FD del socket: <%d>", socket);
                break;

            case HILO:
                hilo_actual = recibir_hilo(socket);
                ejecutar_ciclo_instruccion();
                break;

            case PROCESO:
                pcb_actual = recibir_proceso(socket);
                break;

            case MENSAJE:
                char* respuesta = deserializar_mensaje(socket);
                if (respuesta == NULL) {
                    log_warning(LOGGER_CPU, "Error al recibir el mensaje.");
                } else {
                    if (strcmp(respuesta, "OK") == 0) {
                        log_info(LOGGER_CPU, "Se pudo escribir en memoria");
                    } else {
                        log_warning(LOGGER_CPU, "Error al escribir en memoria.");
                    }
                    free(respuesta); // Liberar la memoria del mensaje recibido
                }
                break;

            case INSTRUCCION:
                sem_wait(&sem_mutex_globales);
                t_instruccion* inst = recibir_instruccion(socket);
                instruccion_actual = inst;
                sem_post(&sem_instruccion);
                sem_post(&sem_mutex_globales);
                break;

            case SOLICITUD_BASE_MEMORIA: 
                uint32_t pid_bm = recibir_pid(socket);
                uint32_t base = consultar_base_particion(pid_bm);

                sem_wait(&sem_mutex_globales);
                base_pedida = base;
                sem_post(&sem_mutex_globales);

                log_info(LOGGER_CPU, "Base recibida: %d para PID: %d", base, pid_bm);
                sem_post(&sem_base); // Desbloquea al hilo que espera
                break;

            case SOLICITUD_LIMITE_MEMORIA: 
                uint32_t pid_lm = recibir_pid(socket);
                uint32_t limite = consultar_limite_particion(pid_lm);

                sem_wait(&sem_mutex_globales);
                limite_pedido = limite;
                sem_post(&sem_mutex_globales);

                log_info(LOGGER_CPU, "Límite recibido: %d para PID: %d", limite, pid_lm);
                sem_post(&sem_limite); // Desbloquea al hilo que espera
                break;
            
            case PEDIDO_READ_MEM:
                uint32_t valor = recibir_valor_de_memoria(socket); // Función para recibir el valor
                sem_wait(&sem_mutex_globales);
                valor_memoria = valor;
                sem_post(&sem_mutex_globales);

                log_info(LOGGER_CPU, "Valor recibido: %d", valor_memoria);
                sem_post(&sem_valor_memoria);
                break;

            default:
                log_error(LOGGER_CPU, "Codigo de operacion desconocida: %d", cod);
                break;
        }
    }

    log_warning(LOGGER_CPU, "Finalizando conexión con el cliente.");
    close(socket); // Cerrar el socket del cliente
    free(args->server_name);
   
    return NULL;
}

void* procesar_conexion_interrupt(void* void_args) {
    t_procesar_conexion_args *args = (t_procesar_conexion_args *)void_args;
    //t_log *logger = args->log;
    int socket = args->fd;
    //char* server_name = args->server_name;
    free(args);
    
    op_code cod;
    while(1) {
        if(socket < 0) {
            log_error(LOGGER_CPU, "Socket inválido: %d", socket);
            break;
        }
        // Recibir código de operación
        ssize_t bytes_recibidos = recv(socket, &cod, sizeof(op_code), MSG_WAITALL);
        log_warning(LOGGER_CPU, "ME llego el codigo nro %d", cod);
        if (bytes_recibidos != sizeof(op_code)) {
            log_error(LOGGER_CPU, "Error al recibir código de operación, bytes recibidos: %zd", bytes_recibidos);
            break;
        }
        log_info(LOGGER_CPU, "Se recibió el código de operación: %d", cod);

        t_paquete* paquete = recibir_paquete(socket);

        switch (cod) {
            case HANDSHAKE_interrupt:
                log_info(LOGGER_CPU, "## KERNEL_INTERRUPT Conectado - FD del socket: <%d>", socket);
                break;

            case FINALIZACION_QUANTUM:
                hay_interrupcion = true;
                break;
            
            default:
                log_error(LOGGER_CPU, "Codigo de operacion desconocida: %d", cod);
                break;
        }

        eliminar_paquete(paquete);

    }

    log_warning(LOGGER_CPU, "Finalizando conexión con el cliente.");
    close(socket); // Cerrar el socket del cliente 
    free(args->server_name);

    return NULL;
}

void iniciar_semaforos() {
    sem_init(&sem_base, 0, 0);
    sem_init(&sem_limite, 0, 0);
    sem_init(&sem_valor_memoria, 0, 0);
    sem_init(&sem_instruccion, 0, 0);
    sem_init(&sem_mutex_globales, 0, 1);
}

void destruir_semaforos() {
    sem_destroy(&sem_base);
    sem_destroy(&sem_limite);
    sem_destroy(&sem_valor_memoria);
    sem_destroy(&sem_instruccion);
    sem_destroy(&sem_mutex_globales);  
}