#include <include/cpu.h>
#include <include/ciclo_instruccion.h>

int main() {

    inicializar_config("cpu"); 

	//Inicializamos los servidores
    socket_cpu_dispatch = iniciar_servidor(PUERTO_ESCUCHA_DISPATCH, LOGGER_CPU, IP_CPU, "CPU_DISPATCH");
    socket_cpu_interrupt = iniciar_servidor(PUERTO_ESCUCHA_INTERRUPT, LOGGER_CPU, IP_CPU, "CPU_INTERRUPT");
	
	//Conexion de dispatch con kernel
	socket_cpu_dispatch_kernel = esperar_cliente(socket_cpu_dispatch, LOGGER_CPU);
    gestionarConexionConKernelDispatch();

	//Conexion de interrupt con kernel
    socket_cpu_interrupt_kernel = esperar_cliente(socket_cpu_interrupt, LOGGER_CPU);
    gestionarConexionConKernelInterrupt();

	//Nos conectamos a memoria desde dispatch
    socket_cpu_dispatch_memoria = crear_conexion(IP_MEMORIA, PUERTO_MEMORIA);
    enviar_mensaje("Hola MEMORIA, soy CPU DISPATCH", socket_cpu_dispatch_memoria);
    paquete(socket_cpu_dispatch_memoria, LOGGER_CPU);

	//Nos conectamos a memoria desde dispatch
    socket_cpu_interrupt_memoria = crear_conexion(IP_MEMORIA, PUERTO_MEMORIA);
    enviar_mensaje("Hola MEMORIA, soy CPU INTERRUPT", socket_cpu_interrupt_memoria);
    paquete(socket_cpu_interrupt_memoria, LOGGER_CPU);

	int sockets[] = {socket_cpu_dispatch, socket_cpu_interrupt, socket_cpu_dispatch_kernel, socket_cpu_interrupt_kernel, socket_cpu_dispatch_memoria, socket_cpu_interrupt_memoria, -1};
	terminar_programa(CONFIG_CPU, LOGGER_CPU, sockets);

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

int gestionarConexionConKernelDispatch(){
    t_list* lista;
	while (1) {
		int cod_op = recibir_operacion(socket_cpu_dispatch_kernel);
		switch (cod_op) {
		case MENSAJE:
			recibir_mensaje(socket_cpu_dispatch_kernel, LOGGER_CPU);
			break;
		case PAQUETE:
			lista = recibir_paquete(socket_cpu_dispatch_kernel);
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

int gestionarConexionConKernelInterrupt(){
    t_list* lista;
	while (1) {
		int cod_op = recibir_operacion(socket_cpu_interrupt_kernel);
		switch (cod_op) {
		case MENSAJE:
			recibir_mensaje(socket_cpu_interrupt_kernel, LOGGER_CPU);
			break;
		case PAQUETE:
			lista = recibir_paquete(socket_cpu_interrupt_kernel);
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