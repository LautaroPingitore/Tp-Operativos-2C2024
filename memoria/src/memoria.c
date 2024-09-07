#include <include/memoria.h>

int main() {
    
    inicializar_config("memoria");



	log_info(LOGGER_MEMORIA, "la ip de filesys es: %s", IP_FILESYSTEM);
	log_info(LOGGER_MEMORIA, "el puerto de filesys es: %s", PUERTO_FILESYSTEM);

	//Iniciar Servidor
	socket_memoria = iniciar_servidor(PUERTO_ESCUCHA, LOGGER_MEMORIA, IP_MEMORIA, "MEMORIA");

	//Esperar cpu disp y gestionar conexion
	socket_memoria_cpu_dispatch = esperar_cliente(socket_memoria, LOGGER_MEMORIA);
	gestionarConexionConCPUDispatch();
	
	//Esperar cpu int y gestionar conexion
	socket_memoria_cpu_interrupt = esperar_cliente(socket_memoria, LOGGER_MEMORIA);
	gestionarConexionConCPUInterrupt();	

	//Esperar memoria y gestionar conexion
	socket_memoria_kernel = esperar_cliente(socket_memoria, LOGGER_MEMORIA);
	gestionarConexionConKernel();

	//Conectar memoria con filesysten
	socket_memoria_filesystem = crear_conexion(IP_FILESYSTEM, PUERTO_FILESYSTEM);
	enviar_mensaje("Hola FILESYSTEM, soy Memoria", socket_memoria_filesystem);
	paquete(socket_memoria_filesystem, LOGGER_MEMORIA);

	int sockets[] = {socket_memoria, socket_memoria_cpu_dispatch, socket_memoria_cpu_interrupt, socket_memoria_kernel, socket_memoria_filesystem,-1};
	terminar_programa(CONFIG_MEMORIA, LOGGER_MEMORIA, sockets);


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
    PUERTO_FILESYSTEM = config_get_string_value(CONFIG_MEMORIA,"PUERTO_FILESYSTEM");
	TAM_MEMORIA = config_get_string_value(CONFIG_MEMORIA, "TAM_MEMORIA");
	PATH_INSTRUCCIONES = config_get_string_value(CONFIG_MEMORIA, "PATH_INSTRUCCIONES");
	RETARDO_RESPUESTA = config_get_string_value(CONFIG_MEMORIA, "RETARDO_RESPUESTA");

    ESQUEMA = config_get_string_value(CONFIG_MEMORIA,"ESQUMA");
	ALGORITMO_BUSQUEDA = config_get_string_value(CONFIG_MEMORIA, "ALGORITMO_BUSQUEDA");
	PARTICIONES = config_get_array_value(CONFIG_MEMORIA, "PARTICIONES");
    LOG_LEVEL = config_get_string_value(CONFIG_MEMORIA,"LOG_LEVEL");

	IP_MEMORIA = config_get_string_value(CONFIG_MEMORIA,"IP_MEMORIA");
}


int gestionarConexionConKernel(){
	t_list* lista;
	while (1) {
		int cod_op = recibir_operacion(socket_memoria_kernel);
		switch (cod_op) {
		case MENSAJE:
			recibir_mensaje(socket_memoria_kernel, LOGGER_MEMORIA);
			break;
		case PAQUETE:
			lista = recibir_paquete(socket_memoria_kernel);
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
}

int gestionarConexionConCPUDispatch(){
	t_list* lista;
	while (1) {
		int cod_op = recibir_operacion(socket_memoria_cpu_dispatch);
		switch (cod_op) {
		case MENSAJE:
			recibir_mensaje(socket_memoria_cpu_dispatch, LOGGER_MEMORIA);
			break;
		case PAQUETE:
			lista = recibir_paquete(socket_memoria_cpu_dispatch);
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
}

int gestionarConexionConCPUInterrupt(){
	t_list* lista;
	while (1) {
		int cod_op = recibir_operacion(socket_memoria_cpu_interrupt);
		switch (cod_op) {
		case MENSAJE:
			recibir_mensaje(socket_memoria_cpu_interrupt, LOGGER_MEMORIA);
			break;
		case PAQUETE:
			lista = recibir_paquete(socket_memoria_cpu_interrupt);
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

}

void iterator(char* value) {
	log_info(LOGGER_MEMORIA,"%s", value);
}

void terminar_programa(t_config* config, t_log* logger, int sockets[]){
	log_destroy(logger);
	config_destroy(config);
	int n = 0;
//	int longitud = sizeof(sockets) / sizeof(sockets[0]);
	while(sockets[n] != -1){
		liberar_socket(n);
		n ++;
	}
}