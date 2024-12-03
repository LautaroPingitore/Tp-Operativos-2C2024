// #include "include/hello.h"

// // FUNCIONES DE RECIBIR
// t_paquete *recibir_paquete(int socket_cliente) {
// 	t_paquete *paquete = malloc(sizeof(t_paquete));
// 	paquete->buffer = malloc(sizeof(t_buffer));
// 	paquete->buffer->stream = NULL;
// 	paquete->buffer->size = 0;
// 	paquete->codigo_operacion = 0;

// 	if (recv(socket_cliente, &(paquete->buffer->size), sizeof(uint32_t), 0) != sizeof(uint32_t))
// 	{
// 		printf("Error al recibir el tamanio del buffer\n");
// 		return NULL;
// 	}

// 	paquete->buffer->stream = malloc(paquete->buffer->size);

// 	if (recv(socket_cliente, paquete->buffer->stream, paquete->buffer->size, 0) != paquete->buffer->size)
// 	{
// 		printf("Error al recibir el buffer\n");
// 		return NULL;
// 	}

// 	return paquete;
// }

// // SE USA SOLO PARA RECIBIR MENSAJES
// void *recibir_buffer(int *size, int socket_cliente)
// {
// 	void *buffer;

// 	recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
// 	buffer = malloc(*size);
// 	recv(socket_cliente, buffer, *size, MSG_WAITALL);

// 	return buffer;
// }

// int recibir_operacion(int socket_cliente)
// {
// 	int cod_op;
// 	if (recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL) > 0)
// 		return cod_op;
// 	else
// 	{
// 		close(socket_cliente);
// 		return -1;
// 	}
// }

// void recibir_mensaje(int socket_cliente, t_log *logger)
// {
// 	int size;
// 	char *buffer = recibir_buffer(&size, socket_cliente);
// 	log_info(logger, "Me llego el mensaje: %s", buffer);
// 	free(buffer);
// }

// // CREACION Y SERIALIZACION
// void *serializar_paquete(t_paquete *paquete, int bytes)
// {
// 	void *magic = malloc(bytes);
// 	int desplazamiento = 0;

// 	memcpy(magic + desplazamiento, &(paquete->codigo_operacion), sizeof(int));
// 	desplazamiento += sizeof(int);
// 	memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(int));
// 	desplazamiento += sizeof(int);
// 	memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
// 	desplazamiento += paquete->buffer->size;

// 	return magic;
// }

// void enviar_mensaje(char *mensaje, int socket_cliente)
// {
// 	t_paquete *paquete = malloc(sizeof(t_paquete));

// 	paquete->codigo_operacion = MENSAJE;
// 	paquete->buffer = malloc(sizeof(t_buffer));
// 	paquete->buffer->size = strlen(mensaje) + 1;
// 	paquete->buffer->stream = malloc(paquete->buffer->size);
// 	memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

// 	void *a_enviar = malloc(paquete->buffer->size + sizeof(op_cod) + sizeof(uint32_t));
// 	int offset = 0;

// 	memcpy(a_enviar + offset, &(paquete->codigo_operacion), sizeof(op_cod));
// 	offset += sizeof(op_cod);
// 	memcpy(a_enviar + offset, &(paquete->buffer->size), sizeof(uint32_t));
// 	offset += sizeof(uint32_t);
// 	memcpy(a_enviar + offset, paquete->buffer->stream, paquete->buffer->size);
// 	if (send(socket_cliente, a_enviar, paquete->buffer->size + sizeof(op_cod) + sizeof(uint32_t), 0) == -1)
// 	{
// 		free(a_enviar);
// 	}
// 	free(a_enviar);
// 	eliminar_paquete(paquete);
// }

// void crear_buffer(t_paquete *paquete)
// {
// 	paquete->buffer = malloc(sizeof(t_buffer));
// 	paquete->buffer->size = 0;
// 	paquete->buffer->stream = NULL;
// }

// void agregar_a_paquete(t_paquete *paquete, void *valor, int tamanio)
// {
// 	paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio + sizeof(int));

// 	memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio, sizeof(int));
// 	memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), valor, tamanio);

// 	paquete->buffer->size += tamanio + sizeof(int);
// }

// int enviar_paquete(t_paquete *paquete, int socket_cliente)
// {
//     // Calcular el tamaño total del mensaje a enviar
//     size_t total_size = sizeof(op_cod) + sizeof(int) + paquete->buffer->size;

//     // Asignar memoria para el buffer que se va a enviar
//     void *a_enviar = malloc(total_size);
//     if (a_enviar == NULL) {
//         perror("Error al asignar memoria para a_enviar");
//         return;
//     }

//     int offset = 0;

//     // Copiar el código de operación al inicio del buffer
//     memcpy(a_enviar + offset, &(paquete->codigo_operacion), sizeof(op_cod));
//     offset += sizeof(op_cod);

//     // Copiar el tamaño del buffer después del código de operación
//     memcpy(a_enviar + offset, &(paquete->buffer->size), sizeof(int));
//     offset += sizeof(int);

//     // Copiar el stream de datos al final del buffer
//     memcpy(a_enviar + offset, paquete->buffer->stream, paquete->buffer->size);

//     // Enviar el buffer a través del socket
//     ssize_t bytes_enviados = send(socket_cliente, a_enviar, total_size, 0);
//     if (bytes_enviados == -1)
//     {
//         perror("Error al enviar datos");
//         return -1;
//     }

//     // Liberar la memoria asignada para a_enviar
//     free(a_enviar);

//     return 0;
// }

// void eliminar_paquete(t_paquete *paquete)
// {
// 	free(paquete->buffer->stream);
// 	free(paquete->buffer);
// 	free(paquete);
// }

// t_paquete *crear_paquete_con_codigo_de_operacion(op_cod codigo)
// {
// 	t_paquete *paquete = malloc(sizeof(t_paquete));
// 	paquete->codigo_operacion = codigo;
// 	crear_buffer(paquete);
// 	return paquete;
// }

// void liberar_conexion(int socket_cliente)
// {
// 	close(socket_cliente);
// }