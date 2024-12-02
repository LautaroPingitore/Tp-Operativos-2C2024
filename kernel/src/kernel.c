#include "include/gestor.h"

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
    iniciar_comunicaciones(sockets);

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

void iniciar_comunicaciones(int sockets[]) {
    // Conectar con Memoria
    socket_kernel_memoria = crear_conexion(IP_MEMORIA, PUERTO_MEMORIA);
    if(socket_kernel_memoria == -1) {
        log_error(LOGGER_KERNEL, "Error al conectar con memoria");
        terminar_programa(CONFIG_KERNEL, LOGGER_KERNEL, sockets);
        exit(EXIT_FAILURE);
    }
    log_info(LOGGER_KERNEL, "Conexion con Memoria Establecida");

    // Crear hilo para manejar la comunicación con Memoria
    pthread_t hilo_memoria;
    t_procesar_conexion_args* args_memoria = malloc(sizeof(t_procesar_conexion_args));
    args_memoria->fd = socket_kernel_memoria;
    args_memoria->log = LOGGER_KERNEL;
    args_memoria->server_name = "Memoria";
    pthread_create(&hilo_memoria, NULL, procesar_comunicaciones_memoria, (void*) args_memoria);
    pthread_detach(hilo_memoria);  // Para que no sea necesario hacer join

    // Conexión con CPU Dispatch
    socket_kernel_cpu_dispatch = crear_conexion(IP_CPU, PUERTO_CPU_DISPATCH);
    if(socket_kernel_cpu_dispatch == -1) {
        log_error(LOGGER_KERNEL, "Error al conectar con CPU Dispatch");
        terminar_programa(CONFIG_KERNEL, LOGGER_KERNEL, sockets);
        exit(EXIT_FAILURE);
    }
    log_info(LOGGER_KERNEL, "Conexion con CPU Dispatch Establecida");

    // Crear hilo para manejar la comunicación con CPU Dispatch
    pthread_t hilo_cpu_dispatch;
    t_procesar_conexion_args* args_cpu_dispatch = malloc(sizeof(t_procesar_conexion_args));
    args_cpu_dispatch->fd = socket_kernel_cpu_dispatch;
    args_cpu_dispatch->log = LOGGER_KERNEL;
    args_cpu_dispatch->server_name = "CPU Dispatch";
    pthread_create(&hilo_cpu_dispatch, NULL, procesar_comunicaciones_cpu, (void*) args_cpu_dispatch);
    pthread_detach(hilo_cpu_dispatch);  // Para que no sea necesario hacer join

    // Conexión con CPU Interrupt
    socket_kernel_cpu_interrupt = crear_conexion(IP_CPU, PUERTO_CPU_INTERRUPT);
    if(socket_kernel_cpu_interrupt == -1) {
        log_error(LOGGER_KERNEL, "Error al conectar con CPU Interrupt");
        terminar_programa(CONFIG_KERNEL, LOGGER_KERNEL, sockets);
        exit(EXIT_FAILURE);
    }
    log_info(LOGGER_KERNEL, "Conexion con CPU Interrupt Establecida");

    // Crear hilo para manejar la comunicación con CPU Interrupt
    pthread_t hilo_cpu_interrupt;
    t_procesar_conexion_args* args_cpu_interrupt = malloc(sizeof(t_procesar_conexion_args));
    args_cpu_interrupt->fd = socket_kernel_cpu_interrupt;
    args_cpu_interrupt->log = LOGGER_KERNEL;
    args_cpu_interrupt->server_name = "CPU Interrupt";
    pthread_create(&hilo_cpu_interrupt, NULL, procesar_comunicaciones_cpu, (void*) args_cpu_interrupt);
    pthread_detach(hilo_cpu_interrupt);  // Para que no sea necesario hacer join
}

// OJO CON ESTA PORQUE ES EXTREMADAMENTE RARA
void* procesar_comunicaciones_memoria(void* void_args) {
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
        case HANDSHAKE_memoria:
            recibir_mensaje(socket, logger);
            log_info(logger, "Conexion establecida con Memoria");
            break;

        case MENSAJE:
            char* respuesta = (char*)stream;
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

        eliminar_paquete(paquete);
    }

    return NULL;
}

void* procesar_comunicaciones_cpu(void* void_args) {
    t_procesar_conexion_args *args = (t_procesar_conexion_args *)void_args;
    t_log *logger = args->log;
    int socket = args->fd;
    char *server_name = args->server_name;
    free(args);

    while(socket != -1) {
        t_paquete* paquete = recibir_paquete_entero(socket);  // Recibimos el paquete entero
        void* stream = paquete->buffer->stream;
        int size = paquete->buffer->size;

        switch (paquete->codigo_operacion) {
        case HANDSHAKE_cpu:
            recibir_mensaje(socket, logger);
            log_info(logger, "Conexion establecida con %s", server_name);
            break;
        
        case DEVOLVER_CONTROL_KERNEL:
            // Procesar el TCB recibido desde la CPU (cuando el CPU termina de ejecutar una instrucción)
            t_tcb* tcb = deserializar_paquete_tcb(stream, size);
            if(tcb->motivo_desalojo == INTERRUPCION_BLOQUEO) {
                list_remove(cola_exec, 0);

                pthread_mutex_lock(&mutex_cola_ready);
                list_add(cola_ready, tcb);  // Agregar el TCB a la cola de ready
                pthread_mutex_unlock(&mutex_cola_ready);

                intentar_mover_a_execute();  // Intentar ejecutar el siguiente hilo
            }
            break;

        default:
            manejar_syscall(paquete);  // Manejar syscalls para el kernel
            break;
        }

        eliminar_paquete(paquete);  // Liberamos la memoria del paquete después de procesarlo
    }

    return NULL;
}
