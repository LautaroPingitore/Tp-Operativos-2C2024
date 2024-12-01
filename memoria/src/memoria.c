#include "include/gestor.h"

char* PUERTO_ESCUCHA;
char* IP_FILESYSTEM;
char* PUERTO_FILESYSTEM;
int TAM_MEMORIA;
char* PATH_INSTRUCCIONES;
int RETARDO_RESPUESTA;
char* ESQUEMA;
char* ALGORITMO_BUSQUEDA;
char** PARTICIONES_FIJAS;
char* LOG_LEVEL; 
char* IP_MEMORIA;

int socket_memoria;
int socket_memoria_kernel;
int socket_memoria_cpu_dispatch;
int socket_memoria_cpu_interrupt;
int socket_memoria_filesystem;

t_log* LOGGER_MEMORIA;
t_config* CONFIG_MEMORIA;

int main() {
    inicializar_programa();
    manejar_conexiones();

    // Ejecutar el servidor en un bucle principal, esperando solicitudes y procesando respuestas
    int sockets[] = {socket_memoria, socket_memoria_cpu_dispatch, socket_memoria_cpu_interrupt, socket_memoria_kernel, socket_memoria_filesystem, -1};
    terminar_programa(CONFIG_MEMORIA, LOGGER_MEMORIA, sockets);

    return 0;
}

void inicializar_programa() {
    // Inicialización de configuración y logger
    inicializar_config("memoria");
    
    // Inicializar listas de estructuras de procesos y memoria
    inicializar_datos();
    
    // Inicializar particiones de memoria en base al esquema (fijo o dinámico)
	t_list* particiones_fijas = obtener_particiones_fijas(PARTICIONES_FIJAS);
    inicializar_lista_particiones(ESQUEMA, particiones_fijas);

    // Configurar conexiones de memoria a otros módulos
    log_info(LOGGER_MEMORIA, "Inicializando servidor de memoria en el puerto %s", PUERTO_ESCUCHA);
    socket_memoria = iniciar_servidor(PUERTO_ESCUCHA, LOGGER_MEMORIA, IP_MEMORIA, "MEMORIA");

    // Conectar memoria con filesystem
    socket_memoria_filesystem = crear_conexion(IP_FILESYSTEM, PUERTO_FILESYSTEM);
    if (socket_memoria_filesystem < 0) {
        log_error(LOGGER_MEMORIA, "No se pudo conectar con el módulo FileSystem");
        exit(EXIT_FAILURE);
    }
    enviar_mensaje("Hola FILESYSTEM, soy Memoria", socket_memoria_filesystem);
    log_info(LOGGER_MEMORIA, "Conexión con FileSystem establecida exitosamente");
}

void manejar_conexiones() {
    while (1) {
        int socket_cliente = esperar_cliente(socket_memoria, LOGGER_MEMORIA);
        if (socket_cliente == -1) continue;

        pthread_t hilo_conexion;
        t_procesar_conexion_args* args = malloc(sizeof(t_procesar_conexion_args));
        if (!args) {
            log_error(LOGGER_MEMORIA, "Error al asignar memoria para los argumentos del hilo");
            close(socket_cliente);
            continue;
        }

        args->log = LOGGER_MEMORIA;
        args->fd = socket_cliente;
        args->server_name = "Servidor Memoria";

        if (pthread_create(&hilo_conexion, NULL, procesar_conexion_memoria, (void*)args) != 0) {
            log_error(LOGGER_MEMORIA, "Error al crear el hilo para conexión");
            free(args); // Liberar la memoria en caso de error
            close(socket_cliente);
            continue;
        }

        pthread_detach(hilo_conexion); // Separar el hilo para evitar bloqueo
    }
}



void inicializar_config(char *arg) {
	
	char config_path[256];
    strcpy(config_path, "../");
    strcat(config_path, arg);
    strcat(config_path, ".config");

	LOGGER_MEMORIA = iniciar_logger("memoria.log", "Servidor Memoria");
	CONFIG_MEMORIA = iniciar_config(config_path, "MEMORIA");

	PUERTO_ESCUCHA = config_get_string_value(CONFIG_MEMORIA, "PUERTO_ESCUCHA");
    IP_FILESYSTEM = config_get_string_value(CONFIG_MEMORIA,"IP_FILESYSTEM");
    PUERTO_FILESYSTEM = config_get_string_value(CONFIG_MEMORIA,"PUERTO_FILESYSTEM");
	TAM_MEMORIA = config_get_int_value(CONFIG_MEMORIA, "TAM_MEMORIA");
	PATH_INSTRUCCIONES = config_get_string_value(CONFIG_MEMORIA, "PATH_INSTRUCCIONES");
	RETARDO_RESPUESTA = config_get_int_value(CONFIG_MEMORIA, "RETARDO_RESPUESTA");

    ESQUEMA = config_get_string_value(CONFIG_MEMORIA,"ESQUEMA");
	ALGORITMO_BUSQUEDA = config_get_string_value(CONFIG_MEMORIA, "ALGORITMO_BUSQUEDA");
	PARTICIONES_FIJAS = config_get_array_value(CONFIG_MEMORIA, "PARTICIONES");
    LOG_LEVEL = config_get_string_value(CONFIG_MEMORIA,"LOG_LEVEL");

	IP_MEMORIA = config_get_string_value(CONFIG_MEMORIA,"IP_MEMORIA");
}

t_list* obtener_particiones_fijas(char** particiones) {
    t_list* lista_particiones_fijas = list_create();
    for (int i = 0; particiones[i] != NULL; i++) {
        int* tam_particion = malloc(sizeof(int));
        *tam_particion = atoi(particiones[i]);
        list_add(lista_particiones_fijas, tam_particion);
    }
    return lista_particiones_fijas;
}