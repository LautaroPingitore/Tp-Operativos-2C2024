#include "include/gestor.h"

// int main() {

//     inicializar_config("cpu"); 

// 	//Inicializamos los servidores
//     socket_cpu_dispatch = iniciar_servidor(PUERTO_ESCUCHA_DISPATCH, LOGGER_CPU, IP_CPU, "CPU_DISPATCH");
//     socket_cpu_interrupt = iniciar_servidor(PUERTO_ESCUCHA_INTERRUPT, LOGGER_CPU, IP_CPU, "CPU_INTERRUPT");
	
// 	//Conexion de dispatch con kernel
// 	socket_cpu_dispatch_kernel = esperar_cliente(socket_cpu_dispatch, LOGGER_CPU);
//     gestionarConexionConKernelDispatch();

// 	//Conexion de interrupt con kernel
//     socket_cpu_interrupt_kernel = esperar_cliente(socket_cpu_interrupt, LOGGER_CPU);
//     gestionarConexionConKernelInterrupt();

// 	//Nos conectamos a memoria desde dispatch
//     socket_cpu_dispatch_memoria = crear_conexion(IP_MEMORIA, PUERTO_MEMORIA);
//     enviar_mensaje("Hola MEMORIA, soy CPU DISPATCH", socket_cpu_dispatch_memoria);
//     paquete(socket_cpu_dispatch_memoria, LOGGER_CPU);

// 	//Nos conectamos a memoria desde dispatch
//     socket_cpu_interrupt_memoria = crear_conexion(IP_MEMORIA, PUERTO_MEMORIA);
//     enviar_mensaje("Hola MEMORIA, soy CPU INTERRUPT", socket_cpu_interrupt_memoria);
//     paquete(socket_cpu_interrupt_memoria, LOGGER_CPU);

// 	int sockets[] = {socket_cpu_dispatch, socket_cpu_interrupt, socket_cpu_dispatch_kernel, socket_cpu_interrupt_kernel, socket_cpu_dispatch_memoria, socket_cpu_interrupt_memoria, -1};
// 	terminar_programa(CONFIG_CPU, LOGGER_CPU, sockets);

//     return 0;
// }

// int gestionarConexionConKernelDispatch(){
//     t_list* lista;
// 	while (1) {
// 		int cod_op = recibir_operacion(socket_cpu_dispatch_kernel);
// 		switch (cod_op) {
// 		case MENSAJE:
// 			recibir_mensaje(socket_cpu_dispatch_kernel, LOGGER_CPU);
// 			break;
// 		case PAQUETE:
// 			lista = recibir_paquete(socket_cpu_dispatch_kernel);
// 			log_info(LOGGER_CPU, "Me llegaron los siguientes valores:\n");
// 			list_iterate(lista, (void*) iterator);
// 			break;
// 		case -1:
// 			log_error(LOGGER_CPU, "El cliente se desconecto. Terminando servidor");
// 			return EXIT_FAILURE;
// 		default:
// 			log_warning(LOGGER_CPU,"Operacion desconocida. No quieras meter la pata");
// 			break;
// 		}
// 	}
// 	return EXIT_SUCCESS;
   
// }

// int gestionarConexionConKernelInterrupt(){
//     t_list* lista;
// 	while (1) {
// 		int cod_op = recibir_operacion(socket_cpu_interrupt_kernel);
// 		switch (cod_op) {
// 		case MENSAJE:
// 			recibir_mensaje(socket_cpu_interrupt_kernel, LOGGER_CPU);
// 			break;
// 		case PAQUETE:
// 			lista = recibir_paquete(socket_cpu_interrupt_kernel);
// 			log_info(LOGGER_CPU, "Me llegaron los siguientes valores:\n");
// 			list_iterate(lista, (void*) iterator);
// 			break;
// 		case -1:
// 			log_error(LOGGER_CPU, "El cliente se desconecto. Terminando servidor");
// 			return EXIT_FAILURE;
// 		default:
// 			log_warning(LOGGER_CPU,"Operacion desconocida. No quieras meter la pata");
// 			break;
// 		}
// 	}
// 	return EXIT_SUCCESS;
// }

int main(int argc, char* argv[]) {
	if (argc != 2) {
        printf("Uso: %s <config_path>\n", argv[0]);
        return EXIT_FAILURE;
    }

	inicializar_config(argv[1]); // ("cpu");

	// INICIALIZA LAS CONEXIONES
	if (iniciar_conexion_con_kernel(&socket_cpu_dispatch, PUERTO_ESCUCHA_DISPATCH, "DISPATCH") < 0 ||
        iniciar_conexion_con_kernel(&socket_cpu_interrupt, PUERTO_ESCUCHA_INTERRUPT, "INTERRUPT") < 0) {
        cerrar_conexiones();
        return EXIT_FAILURE;
    }

	// GESTIONA LAS CONEXIONES
	if (gestionar_conexion(socket_cpu_dispatch, "DISPATCH") < 0 ||
        gestionar_conexion(socket_cpu_interrupt, "INTERRUPT") < 0) {
        cerrar_conexiones();
        return EXIT_FAILURE;
    }

	cerrar_conexiones();
	return EXIT_SUCCESS;
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

int iniciar_conexion_con_kernel(int socket_cliente, char* puerto, char* tipo) {
	*socket_cliente = iniciar_servidor(puerto, LOGGER_CPU, IP_CPU, tipo);
	if(*socket < 0) {
		log_error(LOGGER_CPU, "Error al iniciar conexión para %s", tipo);
        return -1;
	}
	log_info(LOGGER_CPU, "Conexión iniciada para %s en puerto %s.", tipo, puerto);
    return 0;
}

int gestionar_conexion(int socket_cliente, const char* tipo) {
	while(1) {
		int cod_op = recibir_operacion(socket_cliente);
		if (cod_op < 0) {
            log_error(LOGGER_CPU, "%s desconectado.", tipo);
            return -1;
        }
		switch (cod_op) {
            case MENSAJE:
                recibir_mensaje(socket_kernel, LOGGER_CPU);
                break;
            case PAQUETE:
                t_list* lista = recibir_paquete(socket_kernel);
                log_info(LOGGER_CPU, "%s: Operación recibida.", tipo);
                list_destroy_and_destroy_elements(lista, free);
                break;
            default:
                log_warning(LOGGER_CPU, "%s: Operación desconocida.", tipo);
                break;
        }
	}
	return 0;
}

void cerrar_conexiones() {
    if (socket_cpu_dispatch >= 0) close(socket_cpu_dispatch);
    if (socket_cpu_dispatch_kernel >= 0) close(socket_cpu_dispatch_kernel);
    if (socket_cpu_interrupt >= 0) close(socket_cpu_interrupt);
    if (socket_cpu_interrupt_kernel >= 0) close(socket_cpu_interrupt_kernel);
    if (socket_cpu_dispatch_memoria >= 0) close(socket_cpu_dispatch_memoria);
    if (socket_cpu_interrupt_memoria >= 0) close(socket_cpu_interrupt_memoria);

    config_destroy(CONFIG_CPU);
    log_destroy(LOGGER_CPU);
}
