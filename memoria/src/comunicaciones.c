#include "include/comunicaciones.h"

t_contexto_proceso* lista_contextos = NULL;
int cantidad_procesos = 0;
t_proceso_instrucciones* lista_instrucciones = NULL;

static void procesar_conexion_memoria(void *void_args)
{
    inicializar_datos();
    t_procesar_conexion_args *args = (t_procesar_conexion_args *)void_args;
    t_log *logger = args->log;
    int cliente_socket = args->fd;
    char *server_name = args->server_name;
    free(args);

    op_code cod_op;
    while (cliente_socket != -1)
    {
        if (recv(cliente_socket, &cod_op, sizeof(op_code), 0) != sizeof(op_code))
        {
            log_debug(logger, "Cliente desconectado.\n");
            return;
        }

        switch (cod_op)
        {
            case HANDSHAKE_kernel:
                recibir_mensaje(cliente_socket, logger);
                log_info(logger, "Este deberia ser el canal mediante el cual nos comunicamos con el KERNEL");
                break;

            case PROCESS_CREATE:
                t_proceso_memoria* proceso_nuevo = recibir_proceso_kernel(cliente_socket);
                if (asignar_espacio_memoria(proceso_nuevo) == -1) {
                    enviar_respuesta(cliente_socket, "ERROR: No se pudo asignar memoria");
                    free(proceso_nuevo);
                } else {
                    enviar_respuesta(cliente_socket, "OK");
                    log_info(logger, "Proceso Creado - PID: %d - Tamanio: %d", proceso_nuevo->pid, proceso_nuevo->limite);
                }
                break;

            case PROCESS_EXIT:
                t_proceso_memoria* proceso_a_eliminar = recibir_proceso_kernel(cliente_socket);
                liberar_espacio_memoria(proceso_a_eliminar);
                log_info(logger, "Proceso Destruido - PID: %d - Tamanio %d", proceso_a_eliminar->pid, proceso_a_eliminar->limite);
                free(proceso_a_eliminar);
                enviar_respuesta(cliente_socket, "OK");
                break;

            case THREAD_CREATE:
                recibir_creacion_hilo(cliente_socket);
                enviar_respuesta(cliente_socket, "OK");
                log_info(logger, "Hilo creado exitosamente");
                break;

            case THREAD_EXIT:
                recibir_finalizacion_hilo(cliente_socket);
                enviar_respuesta(cliente_socket, "OK");
                log_info(logger, "Hilo finalizado exitosamente");
                break;

            case DUMP_MEMORY:
                uint32_t pid, tid;
                recv(cliente_socket, &pid, sizeof(uint32_t), 0);
                recv(cliente_socket, &tid, sizeof(uint32_t), 0);

                if (solicitar_archivo_filesystem(pid, tid) == 0) {
                    log_info(logger, "Memory Dump realizado correctamente para PID: %d, TID: %d", pid, tid);
                    enviar_respuesta(cliente_socket, "OK");
                } else {
                    log_error(logger, "Error al realizar Memory Dump para PID: %d, TID: %d", pid, tid);
                    enviar_respuesta(cliente_socket, "ERROR");
                }
                break;

            case HANDSHAKE_cpu:
                recibir_mensaje(cliente_socket, logger);
                log_info(logger, "Este deberia ser el canal mediante el cual nos comunicamos con la CPU");
                break;

            case CONTEXTO:
                recibir_solicitud_contexto(cliente_socket);
                break;

            case ACTUALIZAR_CONTEXTO:
                recibir_actualizacion_contexto(cliente_socket);
                break;

            case READ_MEM:
                recibir_read_mem(cliente_socket);
                break;

            case WRITE_MEM:
                recibir_write_mem(cliente_socket);
                break;

            case PEDIDO_INSTRUCCION:
                recibir_solicitud_instruccion(cliente_socket);
                break;

            case ERROROPCODE:
                log_error(logger, "Cliente desconectado de %s... con cod_op -1", server_name);
                break;

            default:
                log_error(logger, "Algo anduvo mal en el servidor de %s, Cod OP: %d", server_name, cod_op);
                break;
        }
    }

    log_warning(logger, "El cliente se desconectó de %s server", server_name);
    return;
}

// PUEDE ESTAR METIDO EN EL INICILIZAR LISTA DE PARTICIONES
void inicializar_listas() {
    lista_procesos_memoria = list_create();
}

int server_escuchar(t_log *logger, char *server_name, int server_socket)
{
    int cliente_socket = esperar_cliente(server_socket, logger);

    if (cliente_socket != -1)
    {
        pthread_t hilo;
        t_procesar_conexion_args *args = malloc(sizeof(t_procesar_conexion_args));
        args->log = logger;
        args->fd = cliente_socket;
        args->server_name = server_name;
        pthread_create(&hilo, NULL, (void *)procesar_conexion_memoria, (void *)args);
        pthread_detach(hilo);
        return 1;
    }
    return 0;
}
//---------------------------------------|
// CONEXION KERNEL DE CREACION DE PROCESO|
//---------------------------------------|

t_pcb* recibir_proceso_kernel(t_pcb* pcb) {
    // Funciones para obtener el proceso
    int resultado = asignar_espacio_memoria(pcb_memoria, ALGORITMO_BUSQUEDA);
    if(resultado) {
        t_proceso_memoria pcb_memoria;
        pcb_memoria->pid = pcb->PID;
        pcb_memoria->limite = pcb->LIMITE;
        pcb_memoria->contexto = pcb->CONTEXTO
        respuesta_asignacion_kernel();//okey);
        return pcb_memoria;
    } else {
        respuesta_asignacion_kernel();//mal);
    }

}

// archivo gestion_memoria funcion "asignar_espacio"

void respuesta_asignacion_kernel(t_pcb* pcb, int socket_cliente) {

}
// COMUNICACION DE CREACION DE PROCESO TERMINADA

// CONEXION KERNEL DE FINALIZACION DE PROCESO
t_pcb* recibir_proceso_a_terminar(uint32_t pid) {
    // BUSCA EL STRUCT QUE TIENE EL MISMO PID
    // LIBERA EL PROCESO DE MEMORIA
}
// COMUNICACION DE FINALIZACION DE PROCESO

// 


//-----------------------------------------------------------
// CREACION DE HILO
//
void recibir_creacion_hilo(int cliente_socket) {
    uint32_t pid, tid;
    recv(cliente_socket, &pid, sizeof(uint32_t), 0);
    recv(cliente_socket, &tid, sizeof(uint32_t), 0);

    t_contexto_ejecucion* nuevo_contexto = malloc(sizeof(t_contexto_ejecucion));
    nuevo_contexto->registros = malloc(sizeof(t_registros));
    memset(nuevo_contexto->registros, 0, sizeof(t_registros));  // Inicializa registros a 0
    nuevo_contexto->registros->program_counter = 0;

    agregar_contexto(pid, nuevo_contexto);

    log_info(LOGGER_MEMORIA, "Hilo creado - PID: %d, TID: %d", pid, tid);
    enviar_respuesta(cliente_socket, "OK");
}





//-----------------------------------------------------------
//FINALIZACION DE HILO
//
void recibir_finalizacion_hilo(int cliente_socket) {
    uint32_t pid, tid;
    recv(cliente_socket, &pid, sizeof(uint32_t), 0);
    recv(cliente_socket, &tid, sizeof(uint32_t), 0);

    // Buscar y eliminar el contexto del hilo
    t_contexto_ejecucion* contexto_a_eliminar = obtener_contexto(pid);
    if (contexto_a_eliminar != NULL) {
        free(contexto_a_eliminar->registros);
        free(contexto_a_eliminar);
        log_info(LOGGER_MEMORIA, "Hilo finalizado - PID: %d, TID: %d", pid, tid);
        enviar_respuesta(cliente_socket, "OK");
    } else {
        log_error(LOGGER_MEMORIA, "Error al finalizar el hilo - PID: %d, TID: %d", pid, tid);
        enviar_respuesta(cliente_socket, "ERROR");
    }
}



//-----------------------------------------------------------
//MEMORY DUMP
//
void realizar_memory_dump(int cliente_socket) {
    uint32_t pid, tid;
    recv(cliente_socket, &pid, sizeof(uint32_t), 0);
    recv(cliente_socket, &tid, sizeof(uint32_t), 0);

    char* nombre_archivo = malloc(50);
    sprintf(nombre_archivo, "%d-%d-%ld.dmp", pid, tid, time(NULL));
    solicitar_archivo_filesystem(pid, tid);  // Implementar para crear y escribir dump en FileSystem
    
    log_info(LOGGER_MEMORIA, "Memory Dump realizado - Archivo: %s", nombre_archivo);
    enviar_respuesta(cliente_socket, "OK");
    free(nombre_archivo);
}
















// Enviar respuesta a la CPU
void enviar_respuesta(int socket_cpu, char *mensaje) {
    // Enviar el mensaje a través del socket de la CPU
    send(socket_cpu, mensaje, strlen(mensaje) + 1, 0); // +1 para incluir el terminador nulo
}

void recibir_pedido_instruccion(uint32_t* pid, uint32_t* pc, int cliente_socket){
    recv(cliente_socket, pid, sizeof(uint32_t), 0);
    recv(cliente_socket, pc, sizeof(uint32_t), 0);

}

t_instruccion* obtener_instruccion(uint32_t pid, uint32_t pc) {
    for(int i=0; i<cantidad_procesos; i++) {
        if(lista_instrucciones[i].pid == pid) {
           if (pc < lista_instrucciones[i].cantidad_instrucciones) {
                return lista_instrucciones[i].instrucciones[pc];
            }
            break; // Si el PC está fuera de límites
        }
    }
    return NULL;
}

void enviar_instruccion(int cliente_socket, uint32_t pid, uint32_t tid, uint32_t pc) {
    t_instruccion* instruccion = obtener_instruccion(pid, tid, pc);
    
    if (instruccion) {
        send(cliente_socket, instruccion, sizeof(t_instruccion), 0);
        log_info(LOGGER_MEMORIA, "## Obtener instrucción - (PID:TID) - (%d:%d) - Instrucción: %d", pid, tid, instruccion->nombre);
    } else {
        log_error(LOGGER_MEMORIA, "Error: Instrucción no encontrada para PID %d, TID %d en PC %d", pid, tid, pc);
    }
}

//void enviar_instruccion(int cliente_socket, t_instruccion* instruccion){
    // Enviar el nombre de la instrucción
//    send(cliente_socket, &instruccion->nombre, sizeof(nombre_instruccion), 0);
    // Enviar cada parámetro
//    uint32_t tam_param1 = strlen(instruccion->parametro1) + 1;
//    uint32_t tam_param2 = strlen(instruccion->parametro2) + 1;
//
//    send(cliente_socket, &tam_param1, sizeof(uint32_t), 0);
//    send(cliente_socket, instruccion->parametro1, tam_param1, 0);
//    send(cliente_socket, &tam_param2, sizeof(uint32_t), 0);
//    send(cliente_socket, instruccion->parametro2, tam_param2, 0);
//}


void recibir_solicitud_instruccion(int cliente_socket) {
    uint32_t pid, tid, pc;
    recv(cliente_socket, &pid, sizeof(uint32_t), 0);
    recv(cliente_socket, &tid, sizeof(uint32_t), 0);
    recv(cliente_socket, &pc, sizeof(uint32_t), 0);

    enviar_instruccion(cliente_socket, pid, tid, pc);
}

void recibir_set(uint32_t* pid, uint32_t* registro, uint32_t* valor, int cliente_socket){
    recv(cliente_socket, pid, sizeof(uint32_t), 0);
    recv(cliente_socket, registro, sizeof(uint32_t), 0);
    recv(cliente_socket, valor, sizeof(uint32_t), 0);
}

t_contexto_ejecucion* obtener_contexto(uint32_t pid) {
    for(int i=0; i<cantidad_procesos; i++) {
        if(lista_instrucciones[i].pid == pid) {
            return &lista_instrucciones[i].contexo;
        }
    }
    return NULL;
}

void recibir_read_mem(int cliente_socket) {
    uint32_t direccion_fisica;
    recv(cliente_socket, &direccion_fisica, sizeof(uint32_t), 0);
    uint32_t valor = leer_memoria(direccion_fisica);

    send(cliente_socket, &valor, sizeof(uint32_t), 0);
    log_info(LOGGER_MEMORIA, "## Lectura - Dirección Física: %d - Tamaño: %d bytes", direccion_fisica, sizeof(uint32_t));
}

void recibir_write_mem(int cliente_socket) {
    uint32_t direccion_fisica, valor;
    recv(cliente_socket, &direccion_fisica, sizeof(uint32_t), 0);
    recv(cliente_socket, &valor, sizeof(uint32_t), 0);

    escribir_memoria(direccion_fisica, valor);
    send(cliente_socket, "OK", 2, 0);
    log_info(LOGGER_MEMORIA, "## Escritura - Dirección Física: %d - Valor: %d", direccion_fisica, valor);
}


void recibir_sub(uint32_t* pid, uint32_t* registro1, uint32_t* registro2, int cliente_socket){
    recv(cliente_socket, pid, sizeof(uint32_t), 0);
    recv(cliente_socket, registro1, sizeof(uint32_t), 0);
    recv(cliente_socket, registro2, sizeof(uint32_t), 0);
}

void recibir_sum(uint32_t* pid,uint32_t* registro1, uint32_t* registro2, int cliente_socket){
    recv(cliente_socket, pid, sizeof(uint32_t), 0);
    recv(cliente_socket, registro1, sizeof(uint32_t), 0);
    recv(cliente_socket, registro2, sizeof(uint32_t), 0);
}

void recibir_jnz(uint32_t* pid, uint32_t* pc_actual, uint32_t* valor_condicion, int cliente_socket){
    recv(cliente_socket, pid, sizeof(uint32_t), 0);
    recv(cliente_socket, pc_actual, sizeof(uint32_t), 0);
    recv(cliente_socket, valor_condicion, sizeof(uint32_t), 0);
}
void cambiar_pc(uint32_t pid, uint32_t pc_actual){

}

void recibir_log(char mensaje[256], int cliente_socket){
    recv(cliente_socket, mensaje, 256, 0);
}

void agregar_contexto(uint32_t pid, t_contexto_ejecucion* nuevo_contexto) {
    t_contexto_proceso* temp = realloc(lista_contextos, (cantidad_procesos + 1) * sizeof(t_contexto_proceso));
    if (temp == NULL) {
        log_error(LOGGER_MEMORIA, "Error al expandir lista de contextos");
        return;
    }
    lista_contextos = temp;
    lista_contextos[cantidad_procesos].pid = pid;
    lista_contextos[cantidad_procesos].contexto = *nuevo_contexto;
    cantidad_procesos++;
}


void agregar_instrucciones(uint32_t pid, t_instruccion** instrucciones, size_t cantidad) {
    lista_instrucciones = realloc(lista_instrucciones, cantidad_procesos * sizeof(t_proceso_instrucciones));
    lista_instrucciones[cantidad_procesos - 1].pid = pid;
    lista_instrucciones[cantidad_procesos - 1].instrucciones = instrucciones;
    lista_instrucciones[cantidad_procesos - 1].cantidad_instrucciones = cantidad;
}

/*t_proceso_memoria* recibir_proceso_kernel(int cliente_socket) {
    t_proceso_memoria* proceso = malloc(sizeof(t_proceso_memoria));
    recv(cliente_socket, &(proceso->pid), sizeof(uint32_t), 0);
    recv(cliente_socket, &(proceso->limite), sizeof(uint32_t), 0);
    return proceso;
}*/