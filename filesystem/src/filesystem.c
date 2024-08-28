#include <include/filesystem.h>

int main(int argc, char** argv) {

    inicializar_config(argv[1]);
    
    return 0;
}

void inicializar_config(char* arg){

    char config_path[256];
    strcpy(config_path, arg);
    strcat(config_path, ".config");
    
    LOGGER_FILESYSTEM = iniciar_logger("filesystem.config", "FILESYSTEM");
    CONFIG_FILESYSTEM = iniciar_config(config_path,"FILESYSTEM");
    
    PUERTO_ESCUCHA = config_get_int_value(CONFIG_FILESYSTEM, "PUERTO");
    MOUNT_DIR = config_get_string_value(CONFIG_FILESYSTEM, "MOUNT_DIR");
    BLOCK_SIZE = config_get_int_value(CONFIG_FILESYSTEM, "BLOCK_SIZE");
    BLOCK_COUNT = config_get_int_value(CONFIG_FILESYSTEM, "BLOCK_COUNT");
    RETARDO_ACCESO_BLOQUE = config_get_int_value(CONFIG_FILESYSTEM, "RETARDO_ACCESO_BLOQUE");
    LOG_LEVEL = config_get_string_value(CONFIG_FILESYSTEM, "LOG_LEVEL");
    }