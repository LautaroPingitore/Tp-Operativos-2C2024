#include "include/gestor.h"

char* IP_MEMORIA;
char* PUERTO_MEMORIA;
char* PUERTO_ESCUCHA_DISPATCH;
char* PUERTO_ESCUCHA_INTERRUPT;
char* LOG_LEVEL;
char* IP_CPU;

t_log *LOGGER_CPU;
t_config *CONFIG_CPU;

int socket_servidor_dispatch;
int socket_servidor_interrupt;
int socket_cpu_dispatch_kernel;
int socket_cpu_interrupt_kernel;
int socket_cpu_memoria;

t_list* hilos_ejecutados;
t_list* procesos_ejecutados;

pthread_t hilo_servidor_dispatch;
pthread_t hilo_servidor_interrupt;
pthread_t hilo_com_memoria;

uint32_t base_pedida = 0;
uint32_t limite_pedido = 0;
uint32_t valor_memoria = 0;
t_instruccion* instruccion_actual = NULL;

sem_t sem_base, sem_limite, sem_valor_memoria, sem_instruccion;
sem_t sem_proceso_actual;
sem_t sem_mensaje;

bool mensaje_okey = false;

int main() {
    inicializar_config("../configs/cpu");
    log_info(LOGGER_CPU, "Iniciando CPU");

    inicializar_cpu();
    iniciar_conexiones();

    pthread_exit(NULL); // Evita que el hilo principal finalice y permite que los hilos creados sigan ejecutándose

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
    socket_servidor_dispatch = iniciar_servidor(PUERTO_ESCUCHA_DISPATCH, LOGGER_CPU, IP_CPU, "CPU_DISPATCH");
    if (socket_servidor_dispatch == -1) {
        log_error(LOGGER_CPU, "No se pudo iniciar el servidor CPU_DISPATCH.");
        exit(EXIT_FAILURE);
    }

    // Servidor CPU Interrupt 
    socket_servidor_interrupt = iniciar_servidor(PUERTO_ESCUCHA_INTERRUPT, LOGGER_CPU, IP_CPU, "CPU_INTERRUPT");
    if (socket_servidor_interrupt == -1) {
        log_error(LOGGER_CPU, "No se pudo iniciar el servidor CPU_INTERRUPT.");
        exit(EXIT_FAILURE);
    }

    // Conexión con Memoria
    socket_cpu_memoria = crear_conexion(IP_MEMORIA, PUERTO_MEMORIA);
    if (socket_cpu_memoria == -1) {
        log_error(LOGGER_CPU, "No se pudo conectar con el módulo Memoria.");
        exit(EXIT_FAILURE);
    }
    enviar_handshake(socket_cpu_memoria, HANDSHAKE_cpu);

    pthread_create(&hilo_com_memoria, NULL, procesar_conexion_memoria, NULL);
    pthread_detach(hilo_com_memoria);

    // Iniciar hilos de escucha
    pthread_create(&hilo_servidor_dispatch, NULL, escuchar_cpu_dispatch, NULL);
    pthread_detach(hilo_servidor_dispatch);

    pthread_create(&hilo_servidor_interrupt, NULL, escuchar_cpu_interrupt, NULL);
    pthread_detach(hilo_servidor_interrupt);

}

void* escuchar_cpu_dispatch() {
    log_info(LOGGER_CPU, "El hilo de escucha para CPU_DISPATCH ha iniciado.");
    while (server_escuchar("CPU_DISPATCH", socket_servidor_dispatch, hilo_servidor_dispatch)) {
        log_info(LOGGER_CPU, "Conexión procesada en CPU_DISPATCH.");
    }
    log_warning(LOGGER_CPU, "El servidor de CPU_DISPATCH terminó inesperadamente.");
    return NULL;
}

void* escuchar_cpu_interrupt() {
    log_info(LOGGER_CPU, "El hilo de escucha para CPU_INTERRUPT ha iniciado.");
    while (server_escuchar("CPU_INTERRUPT", socket_servidor_interrupt, hilo_servidor_interrupt)) {
        log_info(LOGGER_CPU, "Conexión procesada en CPU_INTERRUPT.");
    }
    log_warning(LOGGER_CPU, "El servidor de CPU_INTERRUPT terminó inesperadamente.");
    return NULL;
}

int server_escuchar(char *server_name, int server_socket, pthread_t hilo_servidor) {
    while (1) {
        int cliente_socket = esperar_cliente(server_socket, LOGGER_CPU);
        if (cliente_socket == -1) {
            log_warning(LOGGER_CPU, "[%s] Error al aceptar conexión. Reintentando...", server_name);
            continue;
        }
        
        if(server_socket == socket_servidor_dispatch) {
            socket_cpu_dispatch_kernel = cliente_socket;
        }
        else if(server_socket == socket_servidor_interrupt){
            socket_cpu_interrupt_kernel = cliente_socket;
        }
        else{
            log_error(LOGGER_CPU,"SOCKET DESCONOCIDO");
        }

        t_procesar_conexion_args *args = malloc(sizeof(t_procesar_conexion_args));
        if (!args) {
            log_error(LOGGER_CPU, "[%s] Error al asignar memoria para las conexiones.", server_name);
            close(cliente_socket);
            continue;
        }

        args->log = LOGGER_CPU;
        args->fd = cliente_socket;
        args->server_name = strdup(server_name);

        pthread_create(&hilo_servidor, NULL, procesar_conexion_cpu, (void*) args);
        pthread_detach(hilo_servidor);
    }

    return 0; // Este punto no se alcanza debido al ciclo infinito.
}

void* procesar_conexion_memoria(void*) {
    op_code cod;
    while(1) {
        ssize_t bytes_recibidos = recv(socket_cpu_memoria, &cod, sizeof(op_code), MSG_WAITALL);
        if (bytes_recibidos != sizeof(op_code)) {
            log_warning(LOGGER_CPU, "CLIENTE DESCONECTADO");
            terminar_cpu();
            break;
        }

        switch(cod) {
            case HANDSHAKE_memoria:
                log_info(LOGGER_CPU, "## MEMORIA Conectado - FD del socket: <%d>", socket_cpu_memoria);
                break;

            case MENSAJE:
                char* respuesta_mensaje = recibir_mensaje(socket_cpu_memoria);
                if(respuesta_mensaje && strcmp(respuesta_mensaje, "OK") == 0) {
                    mensaje_okey = true;
                    sem_post(&sem_mensaje);
                } else if(strcmp(respuesta_mensaje, "SE PUDO ESCRIBIR") == 0){
                    log_info(LOGGER_CPU, "Se pudo escribir en memoria");
                } else {
                    mensaje_okey = false;
                    sem_post(&sem_mensaje);
                }
                free(respuesta_mensaje);
                break;

            case MENSAJE_WRITE_MEM:
                char* respuesta = recibir_mensaje(socket_cpu_memoria);
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

            case CONTEXTO:
                t_proceso_cpu* proceso_recibido = recibir_proceso(socket_cpu_memoria);

                t_proceso_cpu* proceso_lista = esta_proceso_guardado(proceso_recibido);
                if(proceso_lista == NULL) {
                    list_add(procesos_ejecutados, proceso_recibido);
                    pcb_actual = proceso_recibido;
                } else {
                    pcb_actual = proceso_lista;
                }

                sem_post(&sem_proceso_actual);
                break;

            case INSTRUCCION:
                t_instruccion* inst = recibir_instruccion(socket_cpu_memoria);
                instruccion_actual = inst;
                sem_post(&sem_instruccion);
                break;

            case SOLICITUD_BASE_MEMORIA: 
                uint32_t base = recibir_uint_memoria(socket_cpu_memoria);

                base_pedida = base;

                log_info(LOGGER_CPU, "Base recibida: %d para PID: %d", base, hilo_actual->PID_PADRE);
                sem_post(&sem_base); // Desbloquea al hilo que espera
                break;

            case SOLICITUD_LIMITE_MEMORIA: 
                uint32_t limite = recibir_uint_memoria(socket_cpu_memoria);

                limite_pedido = limite;

                log_info(LOGGER_CPU, "Límite recibido: %d para PID: %d", limite, hilo_actual->PID_PADRE);
                sem_post(&sem_limite); // Desbloquea al hilo que espera
                break;
            
            case PEDIDO_READ_MEM:
                uint32_t valor = recibir_valor_de_memoria(socket_cpu_memoria); // Función para recibir el valor
                
                valor_memoria = valor;

                log_info(LOGGER_CPU, "Valor recibido: %d", valor_memoria);
                sem_post(&sem_valor_memoria);
                break;

            default:
                log_error(LOGGER_CPU, "Codigo de operacion desconocido");
        }
    }
    log_info(LOGGER_CPU, "Finalizando conexión con el módulo MEMORIA.");
    close(socket_cpu_memoria);
    return NULL;
}

void* procesar_conexion_cpu(void* void_args) {
    t_procesar_conexion_args *args = (t_procesar_conexion_args *)void_args;
    //t_log *logger = args->log;
    int socket = args->fd;
    //char* server_name = args->server_name;
    free(args);

    op_code cod;
    while(socket != -1) {
        ssize_t bytes_recibidos = recv(socket, &cod, sizeof(op_code), MSG_WAITALL);
        if (bytes_recibidos <= 0) {
            if (bytes_recibidos == 0) {
                log_warning(LOGGER_CPU, "Socket cerrado por el cliente.");
            } else {
                log_error(LOGGER_CPU, "Error al recibir datos.");
            }
            terminar_cpu();
            break; // Salimos del ciclo si el socket está cerrado o hay error
        }
        

        switch(cod) {
            case HANDSHAKE_dispatch:
                log_info(LOGGER_CPU, "## KERNEL_DIPATCH Conectado - FD del socket: <%d>", socket);
                break;

            case HILO:
                t_tcb* hilo_recibido = recibir_hilo(socket);

                t_tcb* hilo_lista = esta_hilo_guardado(hilo_recibido);
                if(hilo_lista == NULL) {
                    list_add(hilos_ejecutados, hilo_recibido);
                    hilo_actual = hilo_recibido;
                } else {
                    hilo_actual = hilo_lista;
                }

                log_info(LOGGER_CPU, "## TID: <%d> - SOLICITO CONTEXTO EJECUCION", hilo_actual->TID);
                pedir_contexto_memoria(socket_cpu_memoria, hilo_actual->PID_PADRE, hilo_actual->TID);

                sem_wait(&sem_proceso_actual);

                pthread_mutex_lock(&mutex_syscall);
                hay_syscall = false;
                pthread_mutex_unlock(&mutex_syscall);
                
                ejecutar_ciclo_instruccion();
                break;

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
    }

    log_warning(LOGGER_CPU, "Finalizando conexión con el cliente.");
    close(socket); // Cerrar el socket del cliente
    free(args->server_name);
   
    return NULL;
}

void inicializar_cpu() {
    sem_init(&sem_base, 0, 0);
    sem_init(&sem_limite, 0, 0);
    sem_init(&sem_valor_memoria, 0, 0);
    sem_init(&sem_instruccion, 0, 0);
    sem_init(&sem_proceso_actual, 0, 0);
    sem_init(&sem_mensaje, 0, 0);
    pthread_mutex_init(&mutex_syscall, NULL);

    hilos_ejecutados = list_create();
    procesos_ejecutados = list_create();
}

void terminar_cpu() {
    destruir_semaforos();
    destruir_colas();

    int sockets[] = {socket_servidor_dispatch, socket_servidor_interrupt, socket_cpu_dispatch_kernel, socket_cpu_interrupt_kernel, socket_cpu_memoria, -1};
    terminar_programa(CONFIG_CPU, LOGGER_CPU, sockets);
    exit(SUCCES);
}

void destruir_colas() {
    for(int i=0; i < list_size(procesos_ejecutados); i++) {
        t_proceso_cpu* pAct = list_remove(procesos_ejecutados, i);
        i--;
        free(pAct->REGISTROS);
        free(pAct);
    }
    list_destroy(procesos_ejecutados);
    
    list_destroy_and_destroy_elements(hilos_ejecutados, free);
}

void destruir_semaforos() {
    sem_destroy(&sem_base);
    sem_destroy(&sem_limite);
    sem_destroy(&sem_valor_memoria);
    sem_destroy(&sem_instruccion);
    sem_destroy(&sem_mensaje);
}
