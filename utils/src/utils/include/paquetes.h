#ifndef PAQUETES_H_
#define PAQUETES_H_

// typedef enum{
//     HANDSHAKE_consola,
//     HANDSHAKE_kernel,
//     HANDSHAKE_memoria,
//     HANDSHAKE_cpu,
//     HANDSHAKE_interrupt,
//     HANDSHAKE_dispatch,
//     HANDSHAKE_in_out,
//     HANDSHAKE_ok_continue,
//     ERROROPCODE,
//     PEDIR_VALOR_MEMORIA,
//     ESCRIBIR_VALOR_MEMORIA,
//     ACTUALIZAR_CONTEXTO,
//     DEVOLVER_CONTROL_KERNEL,
//     LECTURA_MEMORIA,
//     SEGF_FAULT,
//     MENSAJE,
//     PAQUETE,
//     PCB,
//     PROCESS_CREATE,
//     PROCESS_EXIT,
//     INTERRUPCION,
//     CONTEXTO,
//     PEDIDO_RESIZE,
//     INSTRUCCION,
//     PEDIDO_INSTRUCCION,
//     PEDIDO_WAIT,
//     PEDIDO_SIGNAL,
//     ENVIAR_INTERFAZ,
//     CONEXION_INTERFAZ,
//     DESCONEXION_INTERFAZ,
//     FINALIZACION_INTERFAZ,
//     PEDIDO_READ_MEM,
//     PEDIDO_WRITE_MEM,
//     ENVIAR_PAGINA,
//     ENVIAR_DIRECCION_FISICA,
//     ENVIAR_INTERFAZ_STDIN,
//     ENVIAR_INTERFAZ_STDOUT,
//     ENVIAR_INTERFAZ_FS,
//     RECIBIR_DATO_STDIN,
//     FINALIZACION_INTERFAZ_STDIN,
//     FINALIZACION_INTERFAZ_STDOUT,
//     PEDIDO_ESCRIBIR_DATO_STDIN,
//     PEDIDO_A_LEER_DATO_STDOUT,
//     RESPUESTA_STDIN,
//     RESPUESTA_DATO_STDOUT,
//     PEDIDO_COPY_STRING,
//     RESPUESTA_DATO_MOVIN,
//     SOLICITUD_BASE_MEMORIA,
//     SOLICITUD_LIMITE_MEMORIA,
//     MISMO_TAMANIO,
//     RESIZE_OK,
//     FS_CREATE_DELETE,
//     FINALIZACION_INTERFAZ_DIALFS,
//     FS_TRUNCATE,
//     FS_WRITE_READ,
//     DUMP_MEMORY,
//     IO,
//     THREAD_CREATE,
//     THREAD_JOIN,
//     THREAD_CANCEL,
//     MUTEX_CREATE,
//     MUTEX_LOCK,
//     MUTEX_UNLOCK,
//     THREAD_EXIT,
//     PROCESO,
//     HILO,
//     CREAR_ARCHIVO,
//     FINALIZACION_QUANTUM,
//     SOLICITUD_PROCESO
// } op_code;

// typedef struct {
// 	int size;
// 	void* stream;
// } t_buffer;

// typedef struct {
// 	op_code codigo_operacion;
// 	t_buffer* buffer;
// } t_paquete;

// t_paquete* recibir_paquete(int);
// void* recibir_buffer(int*, int);
// int recibir_operacion(int);
// void recibir_mensaje(int, t_log*);
// void* serializar_paquete(t_paquete*, int);
// void enviar_mensaje(char*, int);
// void crear_buffer(t_paquete*);
// void agregar_a_paquete(t_paquete*, void*, int);
// int enviar_paquete(t_paquete*, int);
// void eliminar_paquete(t_paquete*);
// void liberar_conexion(int);

#endif