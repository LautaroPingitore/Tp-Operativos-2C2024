#ifndef COMUNICACIONES_H_
#define COMUNICACIONES_H_

void *memoriaUsuario;

// Definición del tipo para los argumentos de la conexión
typedef struct {
    t_log *log;          // Logger para registrar eventos
    int fd;              // Descriptor de archivo del socket del cliente
    char *server_name;   // Nombre del servidor para logging
} t_procesar_conexion_args;

void inicializar_datos();

// Funciones para manejar el servidor y la comunicación con otros módulos
void procesar_conexion_memoria(void *);
int server_escuchar(t_log *, char *, int );
void enviar_respuesta(int, char*); 

void recibir_pedido_instruccion(uint32_t*, uint32_t*, int);
t_instruccion* obtener_instruccion(uint32_t, uint32_t);
void enviar_instruccion(int, uint32_t , uint32_t , uint32_t );
t_contexto_ejecucion*  obtener_contexto(uint32_t);

void recibir_read_mem(int);
void recibir_write_mem(int);
void recibir_log(char [256], int);

typedef struct {
    uint32_t pid;
    uint32_t base;
    uint32_t limite;
    t_contexto_ejecucion* contexto;
} t_proceso_memoria;

typedef struct {
    uint32_t pid_padre;
    uint32_t tid;
    char* archivo;
} t_hilo_memoria;

typedef struct {
    uint32_t pid;
    t_contexto_ejecucion* contexto;
} t_contexto_proceso;

typedef struct {
    uint32_t tid;
    t_list* instrucciones;
} t_hilo_instrucciones;

t_list* lista_procesos; // TIPO t_proceso_memoria
t_list* lista_hilos; // TIPO t_hilo_memoria
t_list* lista_contextos; // TIPO t_contexto_proceso
t_list* lista_instrucciones; // TIPO t_hilo_instrucciones

// FUNCIONES MEJORADAS 2.0247234214321473216487537826432
t_proceso_memoria* deserializar_proceso(int, void*, int);
t_hilo_memoria* deserializar_hilo_memoria(int, void*, int);
int eliminar_espacio_hilo(int, t_hilo_memoria*, t_contexto_ejecucion*);
void deserializar_dump_memory(uint32_t, uint32_t, void*, int);
void deserializar_solicitud_instruccion(uint32_t, uint32_t, uint32_t, void*, int);
int enviar_instruccion(int, t_instruccion*);
void enviar_valor_leido_cpu(int, uint32_t, uint32_t);
void procesar_solicitud_contexto(int, uint32_t, uint32_t);
int enviar_contexto_cpu(t_contexto_proceso*);
void procesar_actualizacion_contexto(int, uint32_t, uint32_t, t_contexto_ejecucion*)
int  mandar_solicitud_dump_memory(char*, char*, int);

//t_proceso_memoria* recibir_proceso_kernel(int);
void eliminar_proceso_de_lista(uint32_t);
void inicializar_datos();
//void recibir_creacion_hilo(int);
//void recibir_finalizacion_hilo(int);
t_contexto_ejecucion* obtener_contexto(uint32_t);
void eliminar_instrucciones_hilo(uint32_t);
void liberar_hilo(uint32_t);
int solicitar_archivo_filesystem(uint32_t, uint32_t);
void recibir_solicitud_instruccion(int);
char* obtener_contenido_proceso(uint32_t);
t_list* obtener_lista_instrucciones_por_tid(uint32_t);


// FALTA HACER
void agregar_instrucciones_a_lista(uint32_t, char*);
uint32_t leer_memoria(uint32_t direccion_fisica);
void escribir_memoria(uint32_t, uint32_t);

#endif //COMUNICACIONES_H_