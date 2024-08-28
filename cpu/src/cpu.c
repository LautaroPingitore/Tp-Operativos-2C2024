#include <include/cpu.h>

int main(int argc, char** argv) {

    inicializar_config(argv[1]);    
    
    return 0;
}

void inicializar_config(char* arg){

    char config_path[256];
    strcpy(config_path, arg);
    strcat(config_path, ".config");

    LOGGER_CPU = iniciar_logger("cpu.log", "CPU");
    CONFIG_CPU = iniciar_config(config_path, "CPU");
    
    IP_MEMORIA = config_get_string_value(CONFIG_CPU, "IP_MEMORIA");
    PUERTO_MEMORIA = config_get_int_value(CONFIG_CPU, "PUERTO_MEMORIA");
    PUERTO_ESCUCHA_DISPATCH = config_get_int_value(CONFIG_CPU, "PUERTO_ESCUCHA_DISPATCH");
    PUERTO_ESCUCHA_INTERRUPT = config_get_int_value(CONFIG_CPU, "PUERTO_ESCUCHA_INTERRUPT");
    LOG_LEVEL = config_get_string_value(CONFIG_CPU, "LOG_LEVEL");
}