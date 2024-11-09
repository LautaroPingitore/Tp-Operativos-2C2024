#ifndef COMUNICACIONES_H_
#define COMUNICACIONES_H_



void *memoriaUsuario;

// Definición del tipo para los argumentos de la conexión
typedef struct
{
    t_log *log;          // Logger para registrar eventos
    int fd;              // Descriptor de archivo del socket del cliente
    char *server_name;   // Nombre del servidor para logging
} t_procesar_conexion_args;

// Funciones para manejar el servidor y la comunicación con otros módulos
int server_escuchar(t_log *, char *, int );
void enviar_respuesta(int, char*); 
void procesar_conexion_memoria(void *);

void recibir_pedido_instruccion(uint32_t*, uint32_t*, int);
t_instruccion* obtener_instruccion(uint32_t, uint32_t, uint32_t);
void enviar_instruccion(int, uint32_t , uint32_t , uint32_t );
void recibir_set(uint32_t*, uint32_t*, uint32_t*, int);
t_contexto_ejecucion*  obtener_contexto(uint32_t);

void recibir_read_mem(int);
void recibir_write_mem(int);
void recibir_sub(uint32_t*, uint32_t*, uint32_t*, int);
void recibir_sum(uint32_t*, uint32_t*, uint32_t*, int);
void recibir_jnz(uint32_t*, uint32_t* , uint32_t* , int);
void cambiar_pc(uint32_t, uint32_t);
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
    t_contexto_ejecucion contexo;
} t_contexto_proceso;

typedef struct {
    uint32_t tid;
    t_list* instrucciones;
} t_hilo_instrucciones;

t_list* lista_procesos; // TIPO t_proceso_memoria
t_list* lista_hilos; // TIPO t_hilo_memoria
t_list* lista_contextos; // TIPO t_contexto_proceso
t_list* lista_instrucciones; // TIPO t_hilo_instrucciones

t_proceso_memoria* recibir_proceso_kernel(int);
void eliminar_proceso_de_lista(uint32_t);
void inicializar_datos();
void recibir_creacion_hilo(int);
void recibir_finalizacion_hilo(int);
t_contexto_ejecucion* obtener_contexto(uint32_t);
void eliminar_instrucciones_hilo(uint32_t);
int solicitar_archivo_filesystem(uint32_t, uint32_t);
void recibir_solicitud_instruccion(int);
char* obtener_contenido_proceso(uint32_t);
int  mandar_solicitud(char[256], char*, int);


// FALTA HACER
void recibir_solicitud_contexto();
void recibir_actualizacion_contexto();
void agregar_instrucciones_a_lista(uint32_t, char*);
uint32_t leer_memoria(uint32_t direccion_fisica);

#endif //COMUNICACIONES_H_