#ifndef PAQUETES_H_
#define PAQUETES_H_

typedef struct {
	int size;
	void* stream;
} t_buffer;

typedef struct {
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;

t_paquete* recibir_paquete(int);
void* recibir_buffer(int*, int);
int recibir_operacion(int);
void recibir_mensaje(int, t_log*);
void* serializar_paquete(t_paquete*, int);
void enviar_mensaje(char*, int);
void crear_buffer(t_paquete*);
void agregar_a_paquete(t_paquete*, void, int);
int enviar_paquete(t_paquete*, int);
void eliminar_paquete(t_paquete*);
void liberar_conexion(int);

#endif