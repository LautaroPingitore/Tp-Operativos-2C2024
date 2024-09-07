#include <utils/hello.h>

int iniciar_servidor(char* puerto, t_log* logger, char* ip, char* nombreServidor) // Guarda pq capaz el puerto es char*
{
	int socket_servidor;
	int err;

	struct addrinfo hints, *servinfo/*, *p */;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &servinfo);

	// Creamos el socket de escucha del servidor

	socket_servidor = socket(servinfo->ai_family,
                        servinfo->ai_socktype,
                        servinfo->ai_protocol);

	// Asociamos el socket a un puerto

	err = setsockopt(socket_servidor, SOL_SOCKET, SO_REUSEPORT, &(int){1}, sizeof(int));

	if(err == (-1)){
		perror("Error en la funcion setsockopt");
		log_info(logger, "Error en la funcion setsockopt");
	}

	err = bind(socket_servidor, servinfo->ai_addr, servinfo->ai_addrlen);

	if(err == (-1)){
		perror("Error en la funcion bind");
		log_info(logger, "Error en la funcion bind");
	}

	// Escuchamos las conexiones entrantes

	err = listen(socket_servidor, SOMAXCONN);

	if(err == (-1)){
		perror("Error en la funcion listen");
		log_info(logger, "Error en la funcion listen");
	}
	

	freeaddrinfo(servinfo);
	log_info(logger, "El servidor %s esta escuchando. \nIP: %s \nPUERTO: %s \n", nombreServidor, ip, puerto);


	return socket_servidor;
}

int esperar_cliente(int socket_servidor, t_log* logger)
{
	// Aceptamos un nuevo cliente
	int socket_cliente = accept(socket_servidor, NULL, NULL);

	if (socket_cliente == (-1))
	{
		perror("Error al aceptar el cliente");
	}
	
	log_info(logger, "Se conecto un cliente!");

	return socket_cliente;
}

int recibir_operacion(int socket_cliente)
{
	int cod_op;
	if(recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL) > 0)
		return cod_op;
	else
	{
		close(socket_cliente);
		return -1;
	}
}




void recibir_mensaje(int socket_cliente, t_log* logger)
{
	int size;
	char* buffer = recibir_buffer(&size, socket_cliente);
	log_info(logger, "Me llego el mensaje %s", buffer);
	free(buffer);
}

void* recibir_buffer(int* size, int socket_cliente)
{
	void * buffer;

	recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
	buffer = malloc(*size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);

	return buffer;
}

t_list* recibir_paquete(int socket_cliente)
{
	int size;
	int desplazamiento = 0;
	void * buffer;
	t_list* valores = list_create();
	int tamanio;

	buffer = recibir_buffer(&size, socket_cliente);
	while(desplazamiento < size)
	{
		memcpy(&tamanio, buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		char* valor = malloc(tamanio);
		memcpy(valor, buffer+desplazamiento, tamanio);
		desplazamiento+=tamanio;
		list_add(valores, valor);
	}
	free(buffer);
	return valores;
}



t_log* iniciar_logger(char* file_name, char* logger_name)
{
	t_log* nuevo_logger = log_create(file_name, logger_name, 1, LOG_LEVEL_INFO);

	if (nuevo_logger == NULL)
	{
		perror("Error creando el log ");
		printf("%s", logger_name);
		exit(EXIT_FAILURE);
	};
	return nuevo_logger;
}

t_config* iniciar_config(char* file_name, char* config_name)
{
	t_config* nuevo_config = config_create(file_name);

	if (nuevo_config == NULL)
	{
		perror("Error creando el config ");
		printf("%s", config_name);
		exit(EXIT_FAILURE);
	};
	return nuevo_config;
}

/*
t_config* iniciar_config(void)
{
	t_config* nuevo_config = config_create("cliente.config");

	if (nuevo_config == NULL) {
		perror("HUBO UN PROBLEMA EN LA CREACION DEL CONFIG");
		exit(EXIT_FAILURE);
	}

	return nuevo_config;
}
*/

//funciones tp0

int crear_conexion(char *ip, char* puerto)
{
	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &server_info);

	// Ahora vamos a crear el socket.
	int socket_cliente = 0;

	socket_cliente = socket(server_info->ai_family,
                         server_info->ai_socktype,
                         server_info->ai_protocol);

	// Ahora que tenemos el socket, vamos a conectarlo

	int err = connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen);

	if(err == (-1)){
		perror("Error al intentar conectarse al servidor");
	}

	freeaddrinfo(server_info);

	return socket_cliente;
}

void* serializar_paquete(t_paquete* paquete, int bytes)
{
	void * magic = malloc(bytes);
	int desplazamiento = 0;

	memcpy(magic + desplazamiento, &(paquete->codigo_operacion), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
	desplazamiento+= paquete->buffer->size;

	return magic;
}

void enviar_mensaje(char* mensaje, int socket_cliente)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = MENSAJE;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = strlen(mensaje) + 1;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

	int bytes = paquete->buffer->size + 2*sizeof(int);

	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
	eliminar_paquete(paquete);
}


void crear_buffer(t_paquete* paquete)
{
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = 0;
	paquete->buffer->stream = NULL;
}

t_paquete* crear_paquete(void)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = PAQUETE;
	crear_buffer(paquete);
	return paquete;
}

void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio)
{
	paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio + sizeof(int));

	memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio, sizeof(int));
	memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), valor, tamanio);

	paquete->buffer->size += tamanio + sizeof(int);
}

void enviar_paquete(t_paquete* paquete, int socket_cliente)
{
	int bytes = paquete->buffer->size + 2*sizeof(int);
	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
}

void eliminar_paquete(t_paquete* paquete)
{
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}

void liberar_socket(int socket)
{
	close(socket);
}

void paquete(int conexion, t_log* logger)
{
	// Ahora toca lo divertido!
	char* leido;
	t_paquete* paquete = crear_paquete();

	// Leemos y esta vez agregamos las lineas al paquete
	
	leido = readline("> ");

	while(strcmp(leido, "") != 0){
		log_info(logger, "Recibi de consola: %s", leido);
		agregar_a_paquete(paquete, leido, string_length(leido) + 1);
		free(leido);
		leido = readline("> ");
	}
	
	free(leido);

	enviar_paquete(paquete, conexion);

	eliminar_paquete(paquete);

	// ¡No te olvides de liberar las líneas y el paquete antes de regresar!
	
}


/*
void terminar_programa(int conexion, t_log* logger, t_config* config)
{
	Y por ultimo, hay que liberar lo que utilizamos (conexion, log y config) 
	  con las funciones de las commons y del TP mencionadas en el enunciado 

	log_destroy(logger);
	config_destroy(config);
	liberar_conexion(conexion);
}
*/



/*
POSIBLE SOLUCION A LA GENERALIZACION DE GESTIONAR CONEXIONES


// Función original 'iterator', que ahora sólo recibe el valor
void iterator(char* value, t_log* LOG_SERVIDOR) {
    log_info(LOG_SERVIDOR, "%s", value);
}

// Función auxiliar que envuelve la lógica del logger
void iterator_aux(void* value) {
    // Cast del value a char* (elemento de la lista)
    char* str_value = (char*) value;

    // Aquí accedemos al logger usando una variable estática
    extern t_log* LOG_SERVIDOR_GLOBAL;
    iterator(str_value, LOG_SERVIDOR_GLOBAL);
}

// Variable estática para almacenar el logger
t_log* LOG_SERVIDOR_GLOBAL;

int gestionarConexiones(int socket_cliente, t_log* LOG_SERVIDOR) 
{
    t_list* lista;

    // Guardamos el logger en la variable global antes de la iteración
    LOG_SERVIDOR_GLOBAL = LOG_SERVIDOR;

        case PAQUETE:
            lista = recibir_paquete(socket_cliente);
            log_info(LOG_SERVIDOR, "Me llegaron los siguientes valores:\n");

            // Usamos la función auxiliar para iterar sobre la lista
            list_iterate(lista, iterator_aux);

            break;

*/