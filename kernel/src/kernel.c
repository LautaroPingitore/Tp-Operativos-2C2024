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

bool mensaje_okey = false;

pthread_mutex_t mutex_mensaje;

sem_t sem_mensaje;

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
        if (bytes_recibidos != sizeof(op_code)) {
            log_error(LOGGER_KERNEL, "CLIENTE DESCONECTADO");
            break;
        }

        switch (cod) {
            case HANDSHAKE_memoria:
            case HANDSHAKE_dispatch:
            case HANDSHAKE_interrupt:
                log_info(LOGGER_KERNEL, "Handshake recibido de %s.", nombre_modulo);
                break;

            case MENSAJE:
                char* respuesta = recibir_mensaje(socket);
                if (respuesta && strcmp(respuesta, "OK") == 0) {
                    pthread_mutex_lock(&mutex_mensaje);
                    mensaje_okey = true;
                    pthread_mutex_unlock(&mutex_mensaje);
                } else {
                    pthread_mutex_lock(&mutex_mensaje);
                    mensaje_okey = false;
                    pthread_mutex_unlock(&mutex_mensaje);
                }
                sem_post(&sem_mensaje);
                free(respuesta);
                break;
            

            case DEVOLVER_CONTROL_KERNEL: 
                t_tcb* tcb = recibir_hilo(socket);
                if (tcb->motivo_desalojo == INTERRUPCION_BLOQUEO) {
                    t_tcb* tcb_interrumpido = list_remove(cola_exec, 0);
                    log_info(LOGGER_KERNEL, "## (<%d> : <%d>) - Desalojado por fin de quantum", tcb_interrumpido->PID_PADRE, tcb_interrumpido->TID);

                    cpu_libre = true;
                    tcb_interrumpido->motivo_desalojo = ESTADO_INICIAL;

                    free(tcb);

                    mover_hilo_a_ready(tcb_interrumpido);

                    intentar_mover_a_execute();

                }
                break;
            
            case PROCESS_CREATE:
                t_tcb* hilo_pc = list_remove(cola_exec, 0);
                cpu_libre = true;
                if(!hilo_pc) {
                    log_error(LOGGER_KERNEL, "ERROR EN EL HILO ACTUAL");
                }

                t_instruccion* inst_pc = recibir_instruccion(socket);
                int tamanio = atoi(inst_pc->parametro2);
                log_syscall("PROCESS_CREATE", hilo_pc);

                char path_proc[100];
                strcpy(path_proc, "/home/utnso/scripts-pruebas/");
                strcat(path_proc, inst_pc->parametro1);

                syscall_process_create(hilo_pc, path_proc, tamanio, inst_pc->parametro3);
                liberar_instruccion(inst_pc);
                intentar_mover_a_execute();
                break;
            case PROCESS_EXIT:
                t_tcb* hilo_pe = list_remove(cola_exec, 0);
                cpu_libre = true;
                t_instruccion* inst_pe = recibir_instruccion(socket);
                log_syscall("PROCESS_EXIT", hilo_pe);
                syscall_process_exit(hilo_pe->PID_PADRE);
                liberar_instruccion(inst_pe);
                intentar_mover_a_execute();
                break;
            case THREAD_CREATE:
                t_tcb* hilo_tc = list_remove(cola_exec, 0);
                cpu_libre = true;
                t_instruccion* inst_tc = recibir_instruccion(socket);
                int prioridad = atoi(inst_tc->parametro2);
                log_syscall("THREAD_CREATE", hilo_tc);

                char path[100];
                strcpy(path, "/home/utnso/scripts-pruebas/");
                strcat(path, inst_tc->parametro1);

                syscall_thread_create(hilo_tc, hilo_tc->PID_PADRE, path, prioridad);
                liberar_instruccion(inst_tc);
                intentar_mover_a_execute();
                break;
            case THREAD_JOIN:
                t_tcb* hilo_tj = list_remove(cola_exec, 0);
                cpu_libre = true;
                t_instruccion* inst_tj = recibir_instruccion(socket);
                uint32_t tid_esperado = atoi(inst_tj->parametro1);
                log_syscall("THREAD_JOIN", hilo_tj);
                syscall_thread_join(hilo_tj->PID_PADRE, hilo_tj->TID, tid_esperado);
                liberar_instruccion(inst_tj);
                intentar_mover_a_execute();
                break;
            case THREAD_CANCEL:
                t_tcb* hilo_tcl = list_remove(cola_exec, 0);
                cpu_libre = true;
                t_instruccion* inst_tcl = recibir_instruccion(socket);
                uint32_t tid = atoi(inst_tcl->parametro1);
                log_syscall("THREAD_CANCEL", hilo_tcl);
                syscall_thread_cancel(hilo_tcl->PID_PADRE, tid);
                liberar_instruccion(inst_tc);
                intentar_mover_a_execute();
                break;
            case THREAD_EXIT:
                t_tcb* hilo_te = list_remove(cola_exec, 0);
                cpu_libre = true;
                t_instruccion* inst_te = recibir_instruccion(socket);
                log_syscall("THREAD_EXIT", hilo_te);
                syscall_thread_exit(hilo_te->PID_PADRE, hilo_te->TID);
                intentar_mover_a_execute();
                liberar_instruccion(inst_te);
                break;
            case MUTEX_CREATE:
                t_tcb* hilo_mc = list_remove(cola_exec, 0);
                cpu_libre = true;
                t_instruccion* inst_mc = recibir_instruccion(socket);
                log_syscall("MUTEX_CREATE", hilo_mc);
                syscall_mutex_create(hilo_mc, inst_mc->parametro1);
                liberar_instruccion(inst_mc);
                intentar_mover_a_execute();
                break;
            case MUTEX_LOCK:
                t_tcb* hilo_ml = list_remove(cola_exec, 0);
                cpu_libre = true;
                t_instruccion* inst_ml = recibir_instruccion(socket);
                log_syscall("MUTEX_LOCK", hilo_ml);
                syscall_mutex_lock(hilo_ml, inst_ml->parametro1);
                liberar_instruccion(inst_ml);
                intentar_mover_a_execute();
                break;
            case MUTEX_UNLOCK:
                t_tcb* hilo_mu = list_remove(cola_exec, 0);
                cpu_libre = true;
                t_instruccion* inst_mu = recibir_instruccion(socket);
                log_syscall("MUTEX_UNLOCK", hilo_mu);
                syscall_mutex_unlock(hilo_mu, inst_mu->parametro1);
                liberar_instruccion(inst_mu);
                break;
            case DUMP_MEMORY:
                t_tcb* hilo_dm = list_remove(cola_exec, 0);
                cpu_libre = true;
                t_instruccion* inst_dm = recibir_instruccion(socket);
                log_syscall("DUMP_MEMORY", hilo_dm);
                syscall_dump_memory(hilo_dm->PID_PADRE, hilo_dm->TID);
                intentar_mover_a_execute();
                liberar_instruccion(inst_dm);
                break;
            case IO:
                t_tcb* hilo_io = list_remove(cola_exec, 0);
                cpu_libre = true;
                t_instruccion* inst_io = recibir_instruccion(socket);
                int milisegundos = atoi(inst_io->parametro1);
                log_syscall("IO", hilo_io);
                syscall_io(hilo_io->PID_PADRE, hilo_io->TID, milisegundos);
                liberar_instruccion(inst_io);
                break;

            default:
                log_warning(LOGGER_KERNEL, "Operación %d esconocida recibida desde %s.", cod, nombre_modulo);
                break;
        }
    }
    log_info(LOGGER_KERNEL, "Finalizando conexión con el módulo %s.", nombre_modulo);
    close(socket);
}

// ====================|
// TERMINAR EL PROGRAMA|
// ====================|
void liberar_instruccion(t_instruccion* inst) {
    free(inst->nombre);
    free(inst->parametro1);
    free(inst->parametro2);
    free(inst);
}

void terminar_kernel() {
    termino_programa = true;
    destruir_mutex_semaforos();
    destruir_colas();

    int sockets[] = {socket_kernel_memoria, socket_kernel_cpu_dispatch, socket_kernel_cpu_interrupt, -1};
    terminar_programa(CONFIG_KERNEL, LOGGER_KERNEL, sockets);
    exit(SUCCES);
}

void destruir_mutex_semaforos() {
    pthread_mutex_destroy(&mutex_pid);
    pthread_mutex_destroy(&mutex_tid);
    pthread_mutex_destroy(&mutex_cola_new);
    pthread_mutex_destroy(&mutex_cola_ready);
    pthread_mutex_destroy(&mutex_cola_exit);
    pthread_mutex_destroy(&mutex_cola_blocked);
    pthread_mutex_destroy(&mutex_mensaje);
    pthread_mutex_destroy(&mutex_cola_exec);
    pthread_mutex_destroy(&mutex_cola_multinivel);

    sem_destroy(&sem_mensaje);
    sem_destroy(&sem_io);
}

void destruir_colas() {
    list_destroy(cola_new);
    list_destroy(cola_exec);
    list_destroy(cola_exit);
    list_destroy(cola_blocked_join);
    list_destroy(cola_blocked_mutex);
    list_destroy(cola_blocked_io);
    if(strcmp(ALGORITMO_PLANIFICACION, "CMN") == 0) {
        destruir_colas_multinivel();
    } else {
        list_destroy(cola_ready);
    }
    list_destroy(tabla_paths);
    list_destroy(tabla_procesos);
    list_destroy(recursos_globales);
}

void destruir_colas_multinivel() {
    for(int i=0; i < list_size(colas_multinivel); i++) {
        t_cola_multinivel* cola = list_get(colas_multinivel, i);
        list_destroy(cola->cola);
        free(cola);
    }
    list_destroy(colas_multinivel);
}