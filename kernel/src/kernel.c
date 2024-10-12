#include <include/kernel.h>

// int main() {

//     inicializar_config("kernel");    
//     log_info(LOGGER_KERNEL, "Iniciando KERNEL\n");


//     //Conectamos con memoria
//     socket_kernel_memoria = crear_conexion(IP_MEMORIA, PUERTO_MEMORIA);
//     log_info(LOGGER_KERNEL, "IP_MEMORIA: %s \nPUERTO_MEMORIA %s", IP_MEMORIA, PUERTO_MEMORIA);
//     enviar_mensaje("Hola MEMORIA, soy Kernel", socket_kernel_memoria);
//     paquete(socket_kernel_memoria, LOGGER_KERNEL);

//     //Conectamos con dispatch
//     socket_kernel_cpu_dispatch = crear_conexion(IP_CPU,PUERTO_CPU_DISPATCH);
//     log_info(LOGGER_KERNEL, "IP_CPU: %s \nPUERTO_CPU_DISPATCH %s", IP_CPU, PUERTO_CPU_DISPATCH);
//     enviar_mensaje("Hola CPU DISPATCH, soy KERNEL" , socket_kernel_cpu_dispatch);
//     paquete(socket_kernel_cpu_dispatch , LOGGER_KERNEL);

//     //Conectamos con interrupt
//     socket_kernel_cpu_interrupt = crear_conexion(IP_CPU,PUERTO_CPU_INTERRUPT);
//     log_info(LOGGER_KERNEL, "IP_CPU: %s \nPUERTO_CPU_INTERRUPT %s", IP_CPU, PUERTO_CPU_INTERRUPT);
//     enviar_mensaje("Hola CPU INTERRUPT, soy KERNEL" , socket_kernel_cpu_interrupt);
//     paquete(socket_kernel_cpu_interrupt , LOGGER_KERNEL);
    
    
//     crear_primer_proceso();


//     int sockets[] = {socket_kernel_memoria, socket_kernel_cpu_dispatch, socket_kernel_cpu_interrupt, -1};
//     terminar_programa(CONFIG_KERNEL, LOGGER_KERNEL, sockets);

//     return 0;
// }

int main(int argc, char* argv[]) {
    // VERIFICACIÓN DE QUE SE PASARON AL MENOS 3 ARGUMENTOS (archivo pseudocódigo, tamaño proceso)
    if(argc < 3) {
        printf("Uso: %s [archivo_pseudocodigo] [tamanio_proceso]\n", argv[0]);
        return -1;
    }

    // OBTENCION DEL ARCHIVO DEL PSEUDOCODIGO Y EL TAMANIO DEL PROCESO
    char* archivo_pseudocodigo = argv[1];
    int tamanio_proceso = atoi(argv[2]);

    // INICIAR CONFIGURACION DE KERNEL
    inicializar_config("kernel");
    log_info(LOGGER_KERNEL, "Iniciando KERNEL \n");

    // INICIAR CONEXIONES
    iniciar_comunicaciones();

    // INICIAR LOS SEMAFOROS Y COLAS
    inicializar_colas_y_mutexs();

    // CREAR PROCESOS INICIAL
    crear_proceso(archivo_pseudocodigo, tamanio_proceso, 0);

    while(1) {
        // LOGICA PARA LA CREACION DE MAS PROCESOS Y USO DE LOS PLANIIFICADORES
    }

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
    QUANTUM = config_get_string_value(CONFIG_KERNEL, "QUANTUM");
    LOG_LEVEL = config_get_string_value(CONFIG_KERNEL, "LOG_LEVEL");
}

void iniciar_comunicaciones() {
    int sockets[] = {socket_kernel_memoria, socket_kernel_cpu_dispatch, socket_kernel_cpu_interrupt, -1};

    // CONECTAR CON MEMORIA
    socket_kernel_memoria = crear_conexion(IP_MEMORIA, PUERTO_MEMORIA);
    //MANEJO ERROR
    if(socket_kernel_memoria == -1) {
        log_error(LOGGER_KERNEL, "Error al conectar con memoria");
        terminar_programa(CONFIG_KERNEL, LOGGER_KERNEL, sockets);
        exit(EXIT_FAILURE);
    }

    log_info(LOGGER_KERNEL, "Conexion con Memoria Establecida");
    log_info(LOGGER_KERNEL, "IP_MEMORIA: %s \nPUERTO_MEMORIA %s", IP_MEMORIA, PUERTO_MEMORIA);

    // CONEXION CON CPU DISPATCH
    socket_kernel_cpu_dispatch = crear_conexion(IP_CPU, PUERTO_CPU_DISPATCH);
    // MANEJO ERROR
    if(socket_kernel_cpu_dispatch == -1) {
        log_error(LOGGER_KERNEL, "Error al conectar con CPU Dispatch");
        terminar_programa(CONFIG_KERNEL, LOGGER_KERNEL, sockets);
        exit(EXIT_FAILURE);
    }
        
    log_info(LOGGER_KERNEL, "Conexion con CPU Dispatch Establecida");
    log_info(LOGGER_KERNEL, "IP_CPU: %s \nPUERTO_CPU_DISPATCH %s", IP_CPU, PUERTO_CPU_DISPATCH);
    

    // CONEXION CON CPU INTERRUPT
    socket_kernel_cpu_interrupt = crear_conexion(IP_CPU, PUERTO_CPU_INTERRUPT);
    if (socket_kernel_cpu_interrupt == -1) {
        log_error(LOGGER_KERNEL, "Error al conectar con CPU Interrupt");
        terminar_programa(CONFIG_KERNEL, LOGGER_KERNEL, sockets);
        exit(EXIT_FAILURE);
    }
        
    log_info(LOGGER_KERNEL, "Conexion con CPU Interrupt establecida.");
    log_info(LOGGER_KERNEL, "IP_CPU: %s \nPUERTO_CPU_INTERRUPT %s", IP_CPU, PUERTO_CPU_INTERRUPT);
}