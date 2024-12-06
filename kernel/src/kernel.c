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

pthread_t hilo_com_memoria, hilo_com_dispatch, hilo_com_interrupt;

lista_recursos recursos_globales;
bool se_pudo_asignar=false;

pthread_mutex_t mutex_process_create;

sem_t sem_process_create;

// EL ARHCIVO DE PSEUDOCODIGO ESTA EN UNA CARPETA DE HOME LLAMADA scripts-pruebas
int main(){//int argc, char* argv[]) {
    // VERIFICACIÓN DE QUE SE PASARON AL MENOS 3 ARGUMENTOS (programa, archivo pseudocódigo, tamaño proceso)

    // if(argc != 4) {
    //     printf("Uso: %s [archivo_config] [archivo_pseudocodigo] [tamanio_proceso]\n", argv[0]);
    //     return -1;
    // }

    // OBTENCION DEL ARCHIVO DEL PSEUDOCODIGO Y EL TAMANIO DEL PROCESO
    char* config = "planificacion";//argv[1];
    char pseudo_path[256];
    strcpy(pseudo_path, "/home/utnso/scripts-pruebas/");
    strcat(pseudo_path, "PLANI_PROC");//argv[2]);

    int tamanio_proceso = 32;//atoi(argv[3]);
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

    // Crear hilos para manejar las comunicaciones
    pthread_create(&hilo_com_memoria, NULL, manejar_comunicaciones_memoria, NULL);
    pthread_create(&hilo_com_dispatch, NULL, manejar_comunicaciones_dispatch, NULL);
    pthread_create(&hilo_com_interrupt, NULL, manejar_comunicaciones_interrupt, NULL);

    // CREAR PROCESOS INICIAL Y LO EJECUTA
    crear_proceso(pseudo_path, tamanio_proceso, 0);
    intentar_mover_a_execute();

    // Esperar que los hilos terminen
    pthread_join(hilo_com_memoria, NULL);
    pthread_join(hilo_com_dispatch, NULL);
    pthread_join(hilo_com_interrupt, NULL);

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

void* manejar_comunicaciones_memoria(void* arg) {
    manejar_comunicaciones(socket_kernel_memoria, "MEMORIA");
    return NULL;
}

// Función para manejar comunicaciones con CPU Dispatch en un hilo
void* manejar_comunicaciones_dispatch(void* arg) {
    manejar_comunicaciones(socket_kernel_cpu_dispatch, "CPU_DISPATCH");
    return NULL;
}

// Función para manejar comunicaciones con CPU Interrupt en un hilo
void* manejar_comunicaciones_interrupt(void* arg) {
    manejar_comunicaciones(socket_kernel_cpu_interrupt, "CPU_INTERRUPT");
    return NULL;
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
                log_warning(LOGGER_KERNEL, "Entro a Mensaje");
                //char* respuesta = deserializar_mensaje(socket);
                char* respuesta = recibir_mensaje(socket);
                log_warning(LOGGER_KERNEL, "%s", respuesta);
                if (respuesta && strcmp(respuesta, "OK") == 0) {
                    pthread_mutex_lock(&mutex_process_create);
                    se_pudo_asignar = true;
                    pthread_mutex_unlock(&mutex_process_create);
                }
                log_warning(LOGGER_KERNEL, "A PUNTO DE HACER EL POST");
                sem_post(&sem_process_create);
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
