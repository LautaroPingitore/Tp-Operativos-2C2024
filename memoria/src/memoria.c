#include <include/memoria.h>

int main() {
    
    inicializar_config("memoria");

	int socket_memoria = iniciar_servidor(PUERTO_ESCUCHA, LOGGER_MEMORIA, IP_MEMORIA, "MEMORIA");
	int socket_kernel = esperar_cliente(socket_memoria, LOGGER_MEMORIA);

	t_list* lista;
	while (1) {
		int cod_op = recibir_operacion(socket_kernel);
		switch (cod_op) {
		case MENSAJE:
			recibir_mensaje(socket_kernel, LOGGER_MEMORIA);
			break;
		case PAQUETE:
			lista = recibir_paquete(socket_kernel);
			log_info(LOGGER_MEMORIA, "Me llegaron los siguientes valores:\n");
			list_iterate(lista, (void*) iterator);
			break;
		case -1:
			log_error(LOGGER_MEMORIA, "El cliente se desconecto. Terminando servidor");
			return EXIT_FAILURE;
		default:
			log_warning(LOGGER_MEMORIA,"Operacion desconocida. No quieras meter la pata");
			break;
		}
	}
	return EXIT_SUCCESS;

    return 0;
}












void inicializar_config(char *arg)
{
	/*
	char config_path[256];
	strcpy(config_path, "./config/");
	strcat(config_path, arg);
	strcat(config_path, ".config");
*/
	char config_path[256];
    strcpy(config_path, "../");
    strcat(config_path, arg);
    strcat(config_path, ".config");

	LOGGER_MEMORIA = iniciar_logger("memoria.log", "Servidor Memoria");
	CONFIG_MEMORIA = iniciar_config(config_path, "MEMORIA");

	PUERTO_ESCUCHA = config_get_string_value(CONFIG_MEMORIA, "PUERTO_ESCUCHA");
    IP_FILESYSTEM = config_get_string_value(CONFIG_MEMORIA,"IP_FILESYSTEM");
    PUERTO_FILESYSTEM = config_get_string_value(CONFIG_MEMORIA,"PUERTO_FYLESYSTEM");
	TAM_MEMORIA = config_get_string_value(CONFIG_MEMORIA, "TAM_MEMORIA");
	PATH_INSTRUCCIONES = config_get_string_value(CONFIG_MEMORIA, "PATH_INSTRUCCIONES");
	RETARDO_RESPUESTA = config_get_string_value(CONFIG_MEMORIA, "RETARDO_RESPUESTA");

    ESQUEMA = config_get_string_value(CONFIG_MEMORIA,"ESQUMA");
	ALGORITMO_BUSQUEDA = config_get_string_value(CONFIG_MEMORIA, "ALGORITMO_BUSQUEDA");
	PARTICIONES = config_get_array_value(CONFIG_MEMORIA, "PARTICIONES");
    LOG_LEVEL = config_get_string_value(CONFIG_MEMORIA,"LOG_LEVEL");

	IP_MEMORIA = config_get_string_value(CONFIG_MEMORIA,"IP_MEMORIA");
}

void iterator(char* value) {
	log_info(LOGGER_MEMORIA,"%s", value);
}