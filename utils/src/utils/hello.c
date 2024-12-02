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

	printf("ESTA HACIENDO LA RECIBICION DEL BUFFER");
	
	void * buffer;

	recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
	buffer = malloc(*size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);

	printf("llego buffeeeeeeer");

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


t_paquete* recibir_paquete_entero(int socket_cliente) {
	printf("dasdasdasd");
    int size;
    int desplazamiento = 0;
    void *buffer;
    t_paquete* paquete = malloc(sizeof(t_paquete));  // Reservar memoria para el paquete    
	buffer = recibir_buffer(&size, socket_cliente);
    
    // Leer el código de operación
    memcpy(&paquete->codigo_operacion, buffer + desplazamiento, sizeof(op_code));
    desplazamiento += sizeof(op_code);

    // Leer el tamaño del buffer
    int tamanio_buffer;
    memcpy(&tamanio_buffer, buffer + desplazamiento, sizeof(int));
    desplazamiento += sizeof(int);

    // Reservar memoria para el buffer y copiar los datos
    paquete->buffer = malloc(sizeof(t_buffer));  // Asegúrate de definir t_buffer correctamente
    paquete->buffer->size = tamanio_buffer;      // Asigna el tamaño al buffer
    paquete->buffer->stream = malloc(tamanio_buffer);  // Reserva memoria para el stream
    memcpy(paquete->buffer->stream, buffer + desplazamiento, tamanio_buffer);
    
	printf("Checkpoint 3");

    free(buffer);
    return paquete;  // Retorna el puntero a t_paquete
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
	void * paqueteSerializado = malloc(bytes);
	int desplazamiento = 0;

	memcpy(paqueteSerializado + desplazamiento, &(paquete->codigo_operacion), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(paqueteSerializado + desplazamiento, &(paquete->buffer->size), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(paqueteSerializado + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
	desplazamiento+= paquete->buffer->size;

	return paqueteSerializado;
}

void enviar_mensaje(char* mensaje, int socket_cliente) {
	t_paquete* paquete = crear_paquete_con_codigo_operacion(MENSAJE);
	agregar_a_paquete(paquete, &mensaje, sizeof(strlen(mensaje) + 1));
	serializar_paquete(paquete, paquete->buffer->size);

	printf("DADASDASDAS");

	if(enviar_paquete(paquete, socket_cliente) == 0) {
		printf("SE A ENVIADO");
	} else {
		printf("NO SE A ENVIADO");
	}

	eliminar_paquete(paquete);
}


void crear_buffer(t_paquete* paquete)
{
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = 0;
	paquete->buffer->stream = NULL;
}

t_paquete* crear_paquete(void) // PUEDE SER REMPLAZADA POR LA FUNCION CREAR_PAQUETE_CON_CODIGO_DE_OPERACION
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

int enviar_paquete(t_paquete* paquete, int socket_cliente)
{
	int bytes = paquete->buffer->size + 2*sizeof(int);
	void* a_enviar = serializar_paquete(paquete, bytes);

	int resultado = send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);

	if (resultado == -1)
	{
		return -1;
	}

	return 0;
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

t_paquete* crear_paquete_con_codigo_operacion(op_code operacion) {
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = operacion;
	crear_buffer(paquete);
	return paquete;
}

t_tcb* deserializar_paquete_tcb(void* stream, int size) {
	t_tcb* tcb = malloc(sizeof(t_tcb));

	memcpy(&tcb->TID, stream + size, sizeof(uint32_t));
    size += sizeof(uint32_t);

    memcpy(&tcb->PRIORIDAD, stream + size, sizeof(uint32_t));
    size += sizeof(uint32_t);

    memcpy(&tcb->PID_PADRE, stream + size, sizeof(uint32_t));
    size += sizeof(uint32_t);

    memcpy(&tcb->ESTADO, stream + size, sizeof(uint32_t));
	size += sizeof(uint32_t);

	memcpy(&tcb->PC, stream + size, sizeof(uint32_t));

	return tcb;
}

t_instruccion* deserializar_instruccion(void* stream, int offset) {

    // Reservar memoria para la instrucción
    t_instruccion* instruccion = malloc(sizeof(t_instruccion));
    if (!instruccion) {
        fprintf(stderr, "Error: No se pudo asignar memoria para t_instruccion\n");
        return NULL;
    }

    // Deserializar nombre
    memcpy(&instruccion->nombre, (char*)stream + offset, sizeof(instruccion->nombre));
    offset += sizeof(instruccion->nombre);

    // Deserializar parametro1
    uint32_t tamanio_parametro1;
    memcpy(&tamanio_parametro1, (char*)stream + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    instruccion->parametro1 = malloc(tamanio_parametro1);
    if (!instruccion->parametro1) {
        fprintf(stderr, "Error: No se pudo asignar memoria para parametro1\n");
        free(instruccion);
        return NULL;
    }
    memcpy(instruccion->parametro1, (char*)stream + offset, tamanio_parametro1);
    offset += tamanio_parametro1;

    // Deserializar parametro2
    uint32_t tamanio_parametro2;
    memcpy(&tamanio_parametro2, (char*)stream + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    instruccion->parametro2 = malloc(tamanio_parametro2);
    if (!instruccion->parametro2) {
        fprintf(stderr, "Error: No se pudo asignar memoria para parametro2\n");
        free(instruccion->parametro1);
        free(instruccion);
        return NULL;
    }
    memcpy(instruccion->parametro2, (char*)stream + offset, tamanio_parametro2);
    offset += tamanio_parametro2;

    // Deserializar parametro3 (entero)
    memcpy(&instruccion->parametro3, (char*)stream + offset, sizeof(int));
    offset += sizeof(int);

    return instruccion;
}