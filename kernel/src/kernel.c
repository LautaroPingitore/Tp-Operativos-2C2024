#include <include/kernel.h>

int main(int argc, char** argv) {

    inicializar_config(argv[1]);    
    
    return 0;
}

void inicializar_config(char* arg){

    char config_path[256];
    strcpy(config_path, arg);
    strcat(config_path, ".config");

    LOGGER_KERNEL = iniciar_logger("kernel.log", "KERNEL");
    CONFIG_KERNEL = iniciar_config(config_path, "KERNEL");
    
    IP_MEMORIA = config_get_string_value(CONFIG_KERNEL, "IP_MEMORIA");
    PUERTO_MEMORIA = config_get_int_value(CONFIG_KERNEL, "PUERTO_MEMORIA");
    IP_CPU = config_get_string_value(CONFIG_KERNEL, "IP_CPU");
    PUERTO_CPU_DISPATCH = config_get_int_value(CONFIG_KERNEL, "PUERTO_CPU_DISPATCH");
    PUERTO_CPU_INTERRUPT = config_get_int_value(CONFIG_KERNEL, "PUERTO_CPU_INTERRUPT");
    ALGORITMO_PLANIFICACION = config_get_string_value(CONFIG_KERNEL, "ALGORITMO_PLANIFICACION");
    QUANTUM = config_get_int_value(CONFIG_KERNEL, "QUANTUM");
    LOG_LEVEL = config_get_string_value(CONFIG_KERNEL, "lOG_LEVEL");
}
