#include <include/filesystem.h>

int main() {

    inicializar_config("filesystem");

    //Iniciar servidor
    socket_filesystem = iniciar_servidor(PUERTO_ESCUCHA,LOGGER_FILESYSTEM,IP_FILESYSTEM,"FILESYSTEM");

    //Esperar a memoria y gestionar conexion
    socket_filesystem_memoria = esperar_cliente(socket_filesystem, LOGGER_FILESYSTEM);
    gestionarConexionConMemoria();

    return 0;
}

void inicializar_config(char* arg){

    char config_path[256];
    strcpy(config_path, "../");
    strcat(config_path, arg);
    strcat(config_path, ".config");
    
    LOGGER_FILESYSTEM = iniciar_logger("filesystem.config", "FILESYSTEM");
    CONFIG_FILESYSTEM = iniciar_config(config_path,"FILESYSTEM");
    
    PUERTO_ESCUCHA = config_get_string_value(CONFIG_FILESYSTEM, "PUERTO_ESCUCHA");
    MOUNT_DIR = config_get_string_value(CONFIG_FILESYSTEM, "MOUNT_DIR");
    BLOCK_SIZE = config_get_string_value(CONFIG_FILESYSTEM, "BLOCK_SIZE");
    BLOCK_COUNT = config_get_string_value(CONFIG_FILESYSTEM, "BLOCK_COUNT");
    RETARDO_ACCESO_BLOQUE = config_get_string_value(CONFIG_FILESYSTEM, "RETARDO_ACCESO_BLOQUE");
    LOG_LEVEL = config_get_string_value(CONFIG_FILESYSTEM, "LOG_LEVEL");

    IP_FILESYSTEM = config_get_string_value(CONFIG_FILESYSTEM,"IP_FILESYSTEM");
}

int gestionarConexionConMemoria(){

t_list* lista;
while(1){
    int cod_op = recibir_operacion(socket_filesystem_memoria);
    switch(cod_op){
    case MENSAJE:
        recibir_mensaje(socket_filesystem_memoria, LOGGER_FILESYSTEM);
        break;
    case PAQUETE:
        lista = recibir_paquete(socket_filesystem_memoria);
        log_info(LOGGER_FILESYSTEM,"Me llegaron los siguientes valores:\n");
        list_iterate(lista, (void*) iterator);
        break;
    case -1:
        log_error(LOGGER_FILESYSTEM, "El cliente se desconecto. Terminando servidor");
        return EXIT_FAILURE;
    default:
        log_warning(LOGGER_FILESYSTEM,"Operacion desconocida. No quieras meter la pata");
        break;
    }
}
    return EXIT_SUCCESS;
}

void iterator(char* value) {
	log_info(LOGGER_FILESYSTEM,"%s", value);
}