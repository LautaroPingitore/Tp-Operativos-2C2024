#include <utils/hello.h>

int iniciar_servidor(char* puerto, t_log* logger) // Guarda pq capaz el puerto es char*
{
	int socket_servidor;
	int err;

	struct addrinfo hints, *servinfo/*, *p */;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;



	getaddrinfo(NULL, puerto, &hints, &servinfo);

	// Creamos el socket de escucha del servidor

	socket_servidor = socket(servinfo->ai_family,
                        servinfo->ai_socktype,
                        servinfo->ai_protocol);

	// Asociamos el socket a un puerto

	err = setsockopt(socket_servidor, SOL_SOCKET, SO_REUSEPORT, &(int){1}, sizeof(int));

	if(err == (-1)){
		perror("Error en la funcion setsockopt");
	}

	err = bind(socket_servidor, servinfo->ai_addr, servinfo->ai_addrlen);

	if(err == (-1)){
		perror("Error en la funcion bind");
	}

	// Escuchamos las conexiones entrantes

	err = listen(socket_servidor, SOMAXCONN);

	if(err == (-1)){
		perror("Error en la funcion listen");
	}
	

	freeaddrinfo(servinfo);
	log_trace(logger, "Listo para escuchar a mi cliente");

	return socket_servidor;
}

int esperar_cliente(int socket_servidor)
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


/* 
GUARDA CON ESTA CHEEE

void recibir_mensaje(int socket_cliente)
{
	int size;
	char* buffer = recibir_buffer(&size, socket_cliente);
	log_info(logger, "Me llego el mensaje %s", buffer);
	free(buffer);
}

*/