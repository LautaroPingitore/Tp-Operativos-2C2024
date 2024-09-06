#include <include/cpu.h>

int main() {

    inicializar_config("cpu"); 
    socket_cpu = iniciar_servidor(PUERTO_ESCUCHA_DISPATCH, LOGGER_CPU, IP_CPU, "MEMORIA");
	socket_cpu_kernel = esperar_cliente(socket_cpu, LOGGER_CPU);

    gestionarConexionConKernel();

    int socket_cpu_dispatch_memoria = crear_conexion(IP_MEMORIA, PUERTO_MEMORIA);
    enviar_mensaje("Hola MEMORIA, soy CPU DISPATCH", socket_cpu_dispatch_memoria);
    paquete(socket_cpu_dispatch_memoria, LOGGER_CPU);

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


    LOGGER_CPU = iniciar_logger("cpu.log", "CPU");
    CONFIG_CPU = iniciar_config(config_path, "CPU");
    
    IP_MEMORIA = config_get_string_value(CONFIG_CPU, "IP_MEMORIA");
    PUERTO_MEMORIA = config_get_string_value(CONFIG_CPU, "PUERTO_MEMORIA");
    PUERTO_ESCUCHA_DISPATCH = config_get_string_value(CONFIG_CPU, "PUERTO_ESCUCHA_DISPATCH");
    PUERTO_ESCUCHA_INTERRUPT = config_get_string_value(CONFIG_CPU, "PUERTO_ESCUCHA_INTERRUPT");
    LOG_LEVEL = config_get_string_value(CONFIG_CPU, "LOG_LEVEL");

    IP_CPU = config_get_string_value(CONFIG_CPU,"IP_CPU");
}

void iterator(char* value) {
	log_info(LOGGER_CPU,"%s", value);
}


int gestionarConexionConKernel(){
    t_list* lista;
	while (1) {
		int cod_op = recibir_operacion(socket_cpu_kernel);
		switch (cod_op) {
		case MENSAJE:
			recibir_mensaje(socket_cpu_kernel, LOGGER_CPU);
			break;
		case PAQUETE:
			lista = recibir_paquete(socket_cpu_kernel);
			log_info(LOGGER_CPU, "Me llegaron los siguientes valores:\n");
			list_iterate(lista, (void*) iterator);
			break;
		case -1:
			log_error(LOGGER_CPU, "El cliente se desconecto. Terminando servidor");
			return EXIT_FAILURE;
		default:
			log_warning(LOGGER_CPU,"Operacion desconocida. No quieras meter la pata");
			break;
		}
	}
	return EXIT_SUCCESS;
   
}