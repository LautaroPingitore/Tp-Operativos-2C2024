#include "include/servidores.h"

t_log *logger_recibido;

int iniciar_servidor(char* puerto, t_log* logger, char* ip, char* name)
{
	logger_recibido = logger;
	int socket_servidor;
	struct addrinfo hints, *servinfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &servinfo);

	bool conecto = false;

	for (struct addrinfo *p = servinfo; p != NULL; p = p->ai_next)
	{
		socket_servidor = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (socket_servidor == -1)
			continue;

		int enable = 1;
		if (setsockopt(socket_servidor, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
		{
			close(socket_servidor);
			log_error(logger, "setsockopt(SO_REUSEADDR) failed");
			continue;
		}

		if (bind(socket_servidor, p->ai_addr, p->ai_addrlen) == -1)
		{
			close(socket_servidor);
			continue;
		}
		conecto = true;
		break;
	}
	if (!conecto)
	{
		free(servinfo);
		return 0;
	}
	listen(socket_servidor, SOMAXCONN);
	log_info(logger, "Servidor %s escuchando en %s:%s", name, ip, puerto);

	freeaddrinfo(servinfo);
	return socket_servidor;
}

int esperar_cliente(int socket_servidor, t_log *logger)
{
	int socket_cliente = accept(socket_servidor, NULL, NULL);

	if (socket_cliente == (-1))
	{
		perror("Error al aceptar el cliente");
	}
	
	log_info(logger, "Se conecto un cliente!");

	return socket_cliente;
}

void iterator(char *value)
{
	log_info(logger_recibido, "%s", value);
}

t_log *iniciar_logger(char *file_name, char *name)
{
	t_log *nuevo_logger;
	nuevo_logger = log_create(file_name, name, 1, LOG_LEVEL_INFO); // LOG_LEVEL_TRACE
	if (nuevo_logger == NULL)
	{
		perror("Error creando el log ");
		printf("%s", name);
		exit(EXIT_FAILURE);
	};
	return nuevo_logger;
}

t_config *iniciar_config(char *file_name, char *name)
{
	t_config *nuevo_config;
	nuevo_config = config_create(file_name);
	if (nuevo_config == NULL)
	{
		perror("Error creando el config ");
		printf("%s", name);
		exit(EXIT_FAILURE);
	};
	return nuevo_config;
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

int crear_conexion(char *ip, char *puerto)
{
	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &server_info);

	// Ahora vamos a crear el socket.
	int socket_cliente = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

	// Ahora que tenemos el socket, vamos a conectarlo
	int err = connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen);

    if(err == (-1)){
		perror("Error al intentar conectarse al servidor");
	}

	freeaddrinfo(server_info);

	return socket_cliente;
}

void liberar_socket(int socket)
{
	close(socket);
}