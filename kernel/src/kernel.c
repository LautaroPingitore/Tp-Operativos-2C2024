#include <include/kernel.h>

int main() {

    inicializar_config("kernel");    
    log_info(LOGGER_KERNEL, "Iniciando KERNEL\n");

    int socket_kernel_memoria = crear_conexion(IP_MEMORIA, PUERTO_MEMORIA);
    enviar_mensaje("Hola MEMORIA, soy Kernel", socket_kernel_memoria);
    paquete(socket_kernel_memoria, LOGGER_KERNEL);

    int socket_kernel_cpu_dispatch = crear_conexion(IP_CPU,PUERTO_CPU_DISPATCH);
    enviar_mensaje("Hola CPU DISPATCH, soy KERNEL" , socket_kernel_cpu_dispatch);
    paquete(socket_kernel_cpu_dispatch , LOGGER_KERNEL);

    int socket_kernel_cpu_interrupt = crear_conexion(IP_CPU,PUERTO_CPU_INTERRUPT);
    enviar_mensaje("Hola CPU INTERRUPT, soy KERNEL" , socket_kernel_cpu_interrupt);
    paquete(socket_kernel_cpu_interrupt , LOGGER_KERNEL);
    

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

