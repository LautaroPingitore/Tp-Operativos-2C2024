#include "include/hello.h"

t_tcb* recibir_hilo(int socket) {
	t_paquete* paquete = recibir_paquete(socket);
	t_tcb* hilo = deserializar_paquete_tcb(paquete->buffer);
	eliminar_paquete(paquete);
	return hilo;
}

t_tcb* deserializar_paquete_tcb(t_buffer* buffer) {
	t_tcb* tcb = malloc(sizeof(t_tcb));
	if(!tcb) {
		printf("Error al crear el tcb");
		return NULL;
	}

	void* stream = buffer->stream;
	int desplazamiento = 0;

	memcpy(&tcb->TID, stream + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);

    memcpy(&tcb->PRIORIDAD, stream + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);

    memcpy(&tcb->PID_PADRE, stream + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);

    memcpy(&tcb->ESTADO, stream + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	memcpy(&tcb->PC, stream + desplazamiento, sizeof(uint32_t));

	return tcb;
}

t_instruccion* deserializar_instruccion(t_buffer* buffer) {
    t_instruccion* instruccion = malloc(sizeof(t_instruccion));
    if (!instruccion) {
        fprintf(stderr, "Error: No se pudo asignar memoria para t_instruccion\n");
        return NULL;
    }

	void* stream = buffer->stream;
	int desplazamiento = 0;
	uint32_t tam_nom, tam_p1, tam_p2;

	memcpy(&tam_nom, stream + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	instruccion->nombre = malloc(tam_nom);
	if(!instruccion->nombre) {
		printf("Error");
		free(instruccion);
		return NULL;
	}
	memcpy(&instruccion->nombre, stream + desplazamiento, tam_nom);
	desplazamiento += tam_nom;

	memcpy(&tam_p1, stream + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	instruccion->parametro1 = malloc(tam_p1);
	if(!instruccion->parametro1) {
		printf("Error");
		free(instruccion->nombre);
		free(instruccion);
		return NULL;
	}
	memcpy(&instruccion->parametro1, stream + desplazamiento, tam_p1);
	desplazamiento += tam_p1;

	memcpy(&tam_p2, stream + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	instruccion->parametro2 = malloc(tam_p2);
	if(!instruccion->parametro2) {
		printf("Error");
		free(instruccion->nombre);
		free(instruccion->parametro1);
		free(instruccion);
		return NULL;
	}
	memcpy(&instruccion->parametro2, stream + desplazamiento, tam_p2);
	desplazamiento += tam_p2;

	memcpy(&instruccion->parametro3, stream + desplazamiento, sizeof(int));
    
    return instruccion;
}

char* deserializar_mensaje(int socket_cliente) {
    // Recibir el tamaño del mensaje
    uint32_t size;
    if (recv(socket_cliente, &size, sizeof(uint32_t), 0) <= 0) {
        return NULL; // Error al recibir el tamaño
    }

    // Reservar memoria para el mensaje
    char* mensaje = malloc(size);
    if (!mensaje) {
        return NULL; // Error al asignar memoria
    }

    // Recibir el mensaje
    if (recv(socket_cliente, mensaje, size, 0) <= 0) {
        free(mensaje);
        return NULL; // Error al recibir el mensaje
    }

    return mensaje; // Retorna el mensaje deserializado
}

uint32_t recibir_pid(int socket) {
    t_paquete* paquete = recibir_paquete(socket);
    uint32_t pid;
    memcpy(&pid, paquete->buffer->stream, sizeof(uint32_t));
    eliminar_paquete(paquete);
    return pid;
}

// SERVIDORES.C
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

t_log* iniciar_logger(char *file_name, char *name)
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

t_config* iniciar_config(char *file_name, char *name)
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


// PAQUETES.C

// FUNCIONES DE RECIBIR
t_paquete *recibir_paquete(int socket_cliente) {
	t_paquete *paquete = malloc(sizeof(t_paquete));
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->stream = NULL;
	paquete->buffer->size = 0;
	paquete->codigo_operacion = 0;

	if (recv(socket_cliente, &(paquete->buffer->size), sizeof(uint32_t), 0) != sizeof(uint32_t))
	{
		printf("Error al recibir el tamanio del buffer\n");
		return NULL;
	}

	paquete->buffer->stream = malloc(paquete->buffer->size);

	if (recv(socket_cliente, paquete->buffer->stream, paquete->buffer->size, 0) != paquete->buffer->size)
	{
		printf("Error al recibir el buffer\n");
		return NULL;
	}

	return paquete;
}

// SE USA SOLO PARA RECIBIR MENSAJES
void *recibir_buffer(int *size, int socket_cliente)
{
	void *buffer;

	recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
	buffer = malloc(*size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);

	return buffer;
}

int recibir_operacion(int socket_cliente)
{
	int cod_op;
	if (recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL) > 0)
		return cod_op;
	else
	{
		close(socket_cliente);
		return -1;
	}
}

void recibir_mensaje(int socket_cliente, t_log *logger)
{
	int size;
	char *buffer = recibir_buffer(&size, socket_cliente);
	log_info(logger, "Me llego el mensaje: %s", buffer);
	free(buffer);
}

// CREACION Y SERIALIZACION
void *serializar_paquete(t_paquete *paquete, int bytes)
{
	void *magic = malloc(bytes);
	int desplazamiento = 0;

	memcpy(magic + desplazamiento, &(paquete->codigo_operacion), sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
	desplazamiento += paquete->buffer->size;

	return magic;
}

void enviar_mensaje(char *mensaje, int socket_cliente)
{
	t_paquete *paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = MENSAJE;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = strlen(mensaje) + 1;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

	void *a_enviar = malloc(paquete->buffer->size + sizeof(op_code) + sizeof(uint32_t));
	int offset = 0;

	memcpy(a_enviar + offset, &(paquete->codigo_operacion), sizeof(op_code));
	offset += sizeof(op_code);
	memcpy(a_enviar + offset, &(paquete->buffer->size), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(a_enviar + offset, paquete->buffer->stream, paquete->buffer->size);
	if (send(socket_cliente, a_enviar, paquete->buffer->size + sizeof(op_code) + sizeof(uint32_t), 0) == -1)
	{
		free(a_enviar);
	}
	free(a_enviar);
	eliminar_paquete(paquete);
}

void crear_buffer(t_paquete *paquete)
{
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = 0;
	paquete->buffer->stream = NULL;
}

void agregar_a_paquete(t_paquete *paquete, void *valor, int tamanio)
{
	paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio + sizeof(int));

	memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio, sizeof(int));
	memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), valor, tamanio);

	paquete->buffer->size += tamanio + sizeof(int);
}

int enviar_paquete(t_paquete *paquete, int socket_cliente)
{
    // Calcular el tamaño total del mensaje a enviar
    size_t total_size = sizeof(op_code) + sizeof(int) + paquete->buffer->size;

    // Asignar memoria para el buffer que se va a enviar
    void *a_enviar = malloc(total_size);
    if (a_enviar == NULL) {
        perror("Error al asignar memoria para a_enviar");
        return -1;
    }

    int offset = 0;

    // Copiar el código de operación al inicio del buffer
    memcpy(a_enviar + offset, &(paquete->codigo_operacion), sizeof(op_code));
    offset += sizeof(op_code);

    // Copiar el tamaño del buffer después del código de operación
    memcpy(a_enviar + offset, &(paquete->buffer->size), sizeof(int));
    offset += sizeof(int);

    // Copiar el stream de datos al final del buffer
    memcpy(a_enviar + offset, paquete->buffer->stream, paquete->buffer->size);

    // Enviar el buffer a través del socket
    ssize_t bytes_enviados = send(socket_cliente, a_enviar, total_size, 0);
    if (bytes_enviados == -1)
    {
        perror("Error al enviar datos");
        return -1;
    }

    // Liberar la memoria asignada para a_enviar
    free(a_enviar);

    return 0;
}

void enviar_handshake(int socket, op_code codigo) {
    t_paquete *paquete = malloc(sizeof(t_paquete));
	
    paquete->codigo_operacion = codigo;
    paquete->buffer = malloc(sizeof(t_buffer));
    paquete->buffer->size = 0;
    paquete->buffer->stream = NULL;

    void *a_enviar = malloc(sizeof(op_code) + sizeof(uint32_t));
    int offset = 0;

    memcpy(a_enviar + offset, &(paquete->codigo_operacion), sizeof(op_code));
    offset += sizeof(op_code);
    memcpy(a_enviar + offset, &(paquete->buffer->size), sizeof(uint32_t));

    if (send(socket, a_enviar, offset, 0) == -1) {
        perror("Error al enviar el handshake");
    }

    free(a_enviar);
    eliminar_paquete(paquete);
	printf("Handshake enviado\n");
	printf("COdigo enviado %d \n", codigo);
}

void eliminar_paquete(t_paquete *paquete)
{
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}

t_paquete *crear_paquete_con_codigo_de_operacion(op_code codigo)
{
	t_paquete *paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = codigo;
	crear_buffer(paquete);
	return paquete;
}

void liberar_conexion(int socket_cliente)
{
	close(socket_cliente);
}