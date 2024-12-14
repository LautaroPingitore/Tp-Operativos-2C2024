#ifndef COMUNICACIONES_H_
#define COMUNICACIONES_H_

extern t_list* lista_procesos; // TIPO t_proceso_memoria
extern t_list* lista_instrucciones; // TIPO t_hilo_instrucciones

extern pthread_mutex_t mutex_procesos;
extern pthread_mutex_t mutex_instrucciones;

t_instruccion* obtener_instruccion(uint32_t,uint32_t, uint32_t);

void recibir_log(char [256], int);

typedef struct {
    uint32_t pid;
    uint32_t base;
    uint32_t limite;
    t_registros* contexto;
} t_proceso_memoria;

typedef struct {
    uint32_t pid_padre;
    uint32_t tid;
    char* archivo;
} t_hilo_memoria;

typedef struct {
    uint32_t pid;
    t_registros* contexto;
} t_contexto_proceso;

typedef struct {
    uint32_t tid;
    uint32_t pid;
    t_list* instrucciones; // tipo t_instrucciones
} t_hilo_instrucciones;

typedef struct {
    uint32_t pid;
    uint32_t tid;
} t_pid_tid;

typedef struct {
    uint32_t pid;
    uint32_t tid;
    t_registros* contexto;
} t_actualizar_contexto;

typedef struct {
    uint32_t pid;
    uint32_t tid;
    uint32_t dire_fisica_wm; 
    uint32_t valor_escribido;
} t_write_mem;

typedef struct {
    uint32_t pid;
    uint32_t tid;
    uint32_t direccion_fisica;
} t_read_mem;


typedef struct {
    uint32_t pid;
    uint32_t tid;
    uint32_t pc;
} t_pedido_instruccion;

// FUNCIONES MEJORADAS 2.0247234214321473216487537826432
void eliminar_espacio_hilo(t_hilo_memoria*);
int enviar_instruccion(int, t_instruccion*);
void enviar_valor_leido_cpu(int, uint32_t, uint32_t);
void procesar_solicitud_contexto(int, uint32_t, uint32_t);
int enviar_contexto_cpu(t_proceso_memoria*);
void procesar_actualizacion_contexto(int, uint32_t, uint32_t, t_registros*);
int  mandar_solicitud_dump_memory(char*, char*, uint32_t);
void liberar_instrucciones(t_list*);
t_proceso_memoria* obtener_proceso_memoria(uint32_t);
t_list* convertir_registros_a_char(t_registros*);


t_proceso_memoria* recibir_proceso(int);
t_proceso_memoria* deserializar_proceso(t_buffer*);
t_hilo_memoria* recibir_hilo_memoria(int);
t_hilo_memoria* deserializar_hilo_memoria(t_buffer*);
t_pid_tid* recibir_identificadores(int);
t_pid_tid* deserializar_identificadores(t_buffer*);
t_actualizar_contexto* recibir_actualizacion(int);
t_actualizar_contexto* deserializar_actualizacion(t_buffer*);
t_write_mem* recibir_write_mem(int);
t_write_mem* deserializar_write_mem(t_buffer*);
t_read_mem* recibir_read_mem(int);
t_read_mem* deserializar_read_mem(t_buffer*);
t_pedido_instruccion* recibir_pedido_instruccion(int);
t_pedido_instruccion* deserializar_pedido_instruccion(t_buffer*);
int enviar_valor_uint_cpu(int, uint32_t, op_code);

//t_proceso_memoria* recibir_proceso_kernel(int);
void eliminar_proceso_de_lista(uint32_t);
void inicializar_datos();
//void recibir_creacion_hilo(int);
//void recibir_finalizacion_hilo(int);
t_registros* obtener_contexto(uint32_t);
void eliminar_instrucciones_hilo(uint32_t,uint32_t);
int solicitar_archivo_filesystem(uint32_t, uint32_t);
void recibir_solicitud_instruccion(int);
char* obtener_contenido_proceso(uint32_t, uint32_t);
t_list* obtener_lista_instrucciones_por_tid(uint32_t, uint32_t);
void agregar_instrucciones_a_lista(uint32_t, uint32_t, char*);
uint32_t leer_memoria(uint32_t direccion_fisica);
void escribir_memoria(uint32_t, uint32_t);

#endif //COMUNICACIONES_H_