#include "include/gestor.h"

t_list* lista_procesos; // TIPO t_proceso_memoria
t_list* lista_hilos; // TIPO t_hilo_memoria
t_list* lista_contextos; // TIPO t_contexto_proceso
t_list* lista_instrucciones; // TIPO t_hilo_instrucciones

void inicializar_datos() {
    lista_procesos = list_create();
    lista_hilos = list_create();
    lista_contextos = list_create();
    lista_instrucciones = list_create();
}

void procesar_conexion_memoria(void *void_args){
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

            case PROCESS_CREATE: // TERMINADO
                t_proceso_memoria* proceso_nuevo = recibir_proceso_kernel(cliente_socket);
                if (asignar_espacio_memoria(proceso_nuevo, ALGORITMO_BUSQUEDA) == -1) {
                    enviar_respuesta(cliente_socket, "ERROR: No se pudo asignar memoria");
                    list_add(lista_procesos, proceso_nuevo);
                    free(proceso_nuevo);
                } else {
                    enviar_respuesta(cliente_socket, "OK");
                    log_info(logger, "Proceso Creado - PID: %d - Tamanio: %d", proceso_nuevo->pid, proceso_nuevo->limite);
                }
                break;

            case PROCESS_EXIT: // TERMINADO
                t_proceso_memoria* proceso_a_eliminar = recibir_proceso_kernel(cliente_socket);
                liberar_espacio_memoria(proceso_a_eliminar);
                eliminar_proceso_de_lista(proceso_a_eliminar->pid);
                log_info(logger, "Proceso Destruido - PID: %d - Tamanio %d", proceso_a_eliminar->pid, proceso_a_eliminar->limite);
                free(proceso_a_eliminar);
                break;

            case THREAD_CREATE: // FALTA TEMA PSEUDOCODIGO
                recibir_creacion_hilo(cliente_socket);
                enviar_respuesta(cliente_socket, "OK");
                log_info(logger, "Hilo creado exitosamente");
                break;

            case THREAD_EXIT: // FALTA VER
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
            
                //uint32_t pid, tid;
                recv(cliente_socket, &pid, sizeof(uint32_t), 0);
                recv(cliente_socket, &tid, sizeof(uint32_t), 0);
                recibir_solicitud_contexto(pid, tid);
                break;

            case ACTUALIZAR_CONTEXTO:
                recibir_actualizacion_contexto(cliente_socket);
                break;
/*
            case READ_MEM:
                recibir_read_mem(cliente_socket);
                break;
*/
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

t_proceso_memoria* recibir_proceso_kernel(int socket_cliente) {
    t_pcb* pcb;
    recv(socket_cliente, &pcb, sizeof(t_pcb), 0); // Checkear
    t_proceso_memoria* pcb_memoria = malloc(sizeof(t_proceso_memoria));
    pcb_memoria->pid = pcb->PID;
    pcb_memoria->limite = pcb->TAMANIO;
    pcb_memoria->contexto = pcb->CONTEXTO;
    return pcb_memoria;
}

void eliminar_proceso_de_lista(uint32_t pid) {
    for (int i = 0; i < list_size(lista_procesos); i++) {
        t_proceso_memoria* proceso_actual = list_get(lista_procesos, i);
        if (pid == proceso_actual->pid) {
            list_remove(lista_procesos, i);
        }
    }
}

//-----------------|
// CREACION DE HILO|
//-----------------|
void recibir_creacion_hilo(int cliente_socket) {
    uint32_t pid, tid;
    char* archivo;
    recv(cliente_socket, &pid, sizeof(uint32_t), 0);
    recv(cliente_socket, &tid, sizeof(uint32_t), 0);
    recv(cliente_socket, &archivo, sizeof(char*), 0);

    t_contexto_ejecucion* nuevo_contexto = malloc(sizeof(t_contexto_ejecucion));
    nuevo_contexto->registros = malloc(sizeof(t_registros));
    memset(nuevo_contexto->registros, 0, sizeof(t_registros));  // Inicializa registros a 0
    nuevo_contexto->registros->program_counter = 0;

    list_add(lista_contextos, nuevo_contexto);

    t_hilo_memoria* hilo_nuevo = malloc(sizeof(t_hilo_memoria));
    hilo_nuevo->pid_padre = pid;
    hilo_nuevo->tid = tid;
    hilo_nuevo->archivo = archivo;

    list_add(lista_hilos, hilo_nuevo);
    
    agregar_instrucciones_a_lista(tid, archivo);

    log_info(LOGGER_MEMORIA, "Hilo creado - PID: %d, TID: %d", pid, tid);
    enviar_respuesta(cliente_socket, "OK");
}

void agregar_instrucciones_a_lista(uint32_t tid, char* archivo) {
    FILE *file = fopen(archivo, "r");
    if (file == NULL) {
        log_error(LOGGER_MEMORIA, "Error al abrir el archivo de pseudocódigo: %s", archivo);
        return;
    }

    char line[256];
    t_list* lst_instrucciones = list_create();
    while (fgets(line, sizeof(line), file) != NULL) {
        t_instruccion* inst = malloc(sizeof(t_instruccion));
        int elementos = sscanf(line, "%s %s %s %d", inst->nombre, inst->parametro1, inst->parametro2, &(inst->parametro3));
        if (elementos == 3) {
            inst->parametro3 = -1;
        } else if (elementos == 2) {
            strcpy(inst->parametro2, "");
            inst->parametro3 = -1;
        } else if (elementos == 1) {
            strcpy(inst->parametro1, "");
            strcpy(inst->parametro2, "");
            inst->parametro3 = -1;
        }
        
        list_add(lst_instrucciones, inst);
        free(inst);
    }

    t_hilo_instrucciones* instrucciones_hilo = malloc(sizeof(t_hilo_instrucciones));
    instrucciones_hilo->tid = tid;
    instrucciones_hilo->instrucciones = lst_instrucciones;
    list_add(lista_instrucciones, instrucciones_hilo);
    list_destroy(lst_instrucciones);  
    fclose(file);
}

//--------------------|
//FINALIZACION DE HILO|
//--------------------|
void recibir_finalizacion_hilo(int cliente_socket) {
    uint32_t pid, tid;
    recv(cliente_socket, &pid, sizeof(uint32_t), 0);
    recv(cliente_socket, &tid, sizeof(uint32_t), 0);

    // Buscar y eliminar el contexto del hilo
    t_contexto_ejecucion* contexto_a_eliminar = obtener_contexto(pid);
    if (contexto_a_eliminar != NULL) {
        free(contexto_a_eliminar->registros);
        free(contexto_a_eliminar);
        eliminar_instrucciones_hilo(tid);
        log_info(LOGGER_MEMORIA, "Hilo finalizado - PID: %d, TID: %d", pid, tid);
        liberar_hilo(tid);
        enviar_respuesta(cliente_socket, "OK");
    } else {
        log_error(LOGGER_MEMORIA, "Error al finalizar el hilo - PID: %d, TID: %d", pid, tid);
        enviar_respuesta(cliente_socket, "ERROR");
    }
}

t_contexto_ejecucion* obtener_contexto(uint32_t pid) {
    for(int i = 0; i < list_size(lista_contextos); i++) {
        t_contexto_proceso* contexto_actual = list_get(lista_contextos, i);
        if(contexto_actual->pid == pid) {
            return contexto_actual->contexto;
        }
    }
    return NULL;
}

void eliminar_instrucciones_hilo(uint32_t tid) {
    for(int i = 0; i < list_size(lista_instrucciones); i++) {
        t_hilo_instrucciones* instruccion_actual = list_get(lista_instrucciones, i);
        if(instruccion_actual->tid == tid) {
            list_remove(lista_instrucciones, i);
            free(instruccion_actual->instrucciones);
            return;
        }
    }
}

void liberar_hilo(uint32_t tid) {
    for(int i=0; i < list_size(lista_hilos); i++) {
        t_hilo_memoria* hilo_actual = list_get(lista_hilos, i);
        if(hilo_actual->tid == tid) {
            free(hilo_actual);
            return;
        }
    }
}

//-----------|
//MEMORY DUMP|
//-----------|
int solicitar_archivo_filesystem(uint32_t pid, uint32_t tid) {
    char nombre_archivo[256];
    time_t t = time(NULL);
    struct tm* tiempo = localtime(&t);
    sprintf(nombre_archivo, "%d-%d-%d", pid, tid, tiempo->tm_sec);
    char* contenido_proceso = obtener_contenido_proceso(tid);
    int tamanio = strlen(contenido_proceso);

    int resultado = mandar_solicitud(nombre_archivo, contenido_proceso, tamanio);

    // Liberar memoria si es necesario
    free(contenido_proceso);

    if (resultado == 0) {
        log_info(LOGGER_MEMORIA, "Solicitud enviada correctamente para el archivo: %s", nombre_archivo);
        return 0;
    } else {
        log_error(LOGGER_MEMORIA, "Error al enviar solicitud para el archivo: %s", nombre_archivo);
        return -1;
    }
}

char* obtener_contenido_proceso(uint32_t tid) {
    t_list* instrucciones = obtener_lista_instrucciones_por_tid(tid);
    if (!instrucciones) return NULL;

    size_t tamanio_total = 0;

    // Calcular el tamaño total necesario para el contenido de las instrucciones
    for (int i = 0; i < list_size(instrucciones); i++) {
        t_instruccion* instruccion = list_get(instrucciones, i);
        tamanio_total += strlen(instruccion->parametro1) + strlen(instruccion->parametro2) + 12; // Considera 10 dígitos para int y separadores
    }

    // Asignar espacio para el contenido y construirlo
    char* contenido = malloc(tamanio_total + 1);
    contenido[0] = '\0';

    char parametro3_str[12]; // Buffer para convertir `parametro3` a cadena
    for (int i = 0; i < list_size(instrucciones); i++) {
        t_instruccion* inst = list_get(instrucciones, i);
        sprintf(parametro3_str, "%d", inst->parametro3); // Convertir `parametro3` a cadena

        strcat(contenido, inst->parametro1);
        strcat(contenido, " ");
        strcat(contenido, inst->parametro2);
        strcat(contenido, " ");
        strcat(contenido, parametro3_str);
        strcat(contenido, "\n");
    }

    return contenido;
}


t_list* obtener_lista_instrucciones_por_tid(uint32_t tid) {
    for (int i = 0; i < list_size(lista_instrucciones); i++) {
        t_hilo_instrucciones* hilo_instrucciones = list_get(lista_instrucciones, i);
        if (hilo_instrucciones->tid == tid) {
            return hilo_instrucciones->instrucciones;
        }
    }

    return NULL;
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



void enviar_instruccion(int cliente_socket, uint32_t pid, uint32_t tid, uint32_t pc) {
    t_instruccion* instruccion = obtener_instruccion(tid, pc);
    
    if (instruccion) {
        send(cliente_socket, instruccion, sizeof(t_instruccion), 0);
        log_info(LOGGER_MEMORIA, "## Obtener instrucción - (PID:TID) - (%d:%d) - Instrucción: %s", pid, tid, instruccion->nombre);
    } else {
        log_error(LOGGER_MEMORIA, "Error: Instrucción no encontrada para PID %d, TID %d en PC %d", pid, tid, pc);
    }
}


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



void recibir_read_mem(int cliente_socket) {
    uint32_t direccion_fisica;
    recv(cliente_socket, &direccion_fisica, sizeof(uint32_t), 0);
    uint32_t valor = leer_memoria(direccion_fisica);

    send(cliente_socket, &valor, sizeof(uint32_t), 0);
    log_info(LOGGER_MEMORIA, "## Lectura - Dirección Física: %d - Tamaño: %lu bytes", direccion_fisica, sizeof(uint32_t));
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

// Lista de instrucciones es de tipo t_proceso_instruccion
// NO SE USA POR AHORA
t_instruccion* obtener_instruccion(uint32_t tid, uint32_t pc) {
    for(int i = 0; i < list_size(lista_instrucciones); i++) {
        t_hilo_instrucciones* instruccion_actual = list_get(lista_instrucciones, i);
        if(instruccion_actual->tid == tid && list_size(instruccion_actual->instrucciones) >= pc) {
            t_instruccion* instruccion_deseada = list_get(instruccion_actual->instrucciones, pc + 1); // CHECKEAR EL + 1
            return instruccion_deseada;
        }
    }
    return NULL;
}


//DESARROLLAARRRRRRRRRRRRRRRRRRRRR
void recibir_solicitud_contexto(uint32_t pid, uint32_t tid) {
    for (int i = 0; i < list_size(lista_contextos); i++) {
        t_contexto_proceso* contexto_proceso = list_get(lista_contextos, i);
        
        if (contexto_proceso->pid == pid) {
            send(socket_memoria_cpu_dispatch, contexto_proceso->contexto, sizeof(t_contexto_ejecucion), 0);
            log_info(LOGGER_MEMORIA, "Contexto enviado al CPU - PID: %d, TID: %d", pid, tid);
            return;
        }
    }

    log_error(LOGGER_MEMORIA, "No se encontró contexto para el PID: %d, TID: %d", pid, tid);
    enviar_respuesta(socket_memoria_cpu_dispatch, "ERROR: Contexto no encontrado");
} 

void recibir_actualizacion_contexto(int socket_cliente) {
    uint32_t pid, tid;
    t_contexto_ejecucion* nuevo_contexto;

    recv(socket_cliente, &pid, sizeof(uint32_t), 0);
    recv(socket_cliente, &tid, sizeof(uint32_t), 0);
    recv(socket_cliente, &nuevo_contexto, sizeof(t_contexto_ejecucion), 0);

    for (int i = 0; i < list_size(lista_contextos); i++) {
        t_contexto_proceso* contexto_proceso = list_get(lista_contextos, i);
        
        if (contexto_proceso->pid == pid) {
            contexto_proceso->contexto = nuevo_contexto;
            log_info(LOGGER_MEMORIA, "Contexto actualizado para PID: %d, TID: %d", pid, tid);
            enviar_respuesta(socket_cliente, "OK");
            return;
        }
    }

    log_error(LOGGER_MEMORIA, "No se encontró contexto para actualizar en PID: %d, TID: %d", pid, tid);
    enviar_respuesta(socket_cliente, "ERROR: Contexto no encontrado");
}

int mandar_solicitud(char* nombre_archivo, char* contenido_proceso, int tamanio) {
    send(socket_memoria_filesystem, nombre_archivo, strlen(nombre_archivo) + 1, 0);
    send(socket_memoria_filesystem, &tamanio, sizeof(int), 0);
    send(socket_memoria_filesystem, contenido_proceso, tamanio, 0);

    close(socket_memoria_filesystem);
    return 1;
}

void agregar_instrucciones(uint32_t tid, t_instruccion** instrucciones, size_t cantidad) {
    t_hilo_instrucciones* hilo_instrucciones = malloc(sizeof(t_hilo_instrucciones));
    hilo_instrucciones->tid = tid;
    hilo_instrucciones->instrucciones = list_create(); 

    for (size_t i = 0; i < cantidad; i++) {
        list_add(hilo_instrucciones->instrucciones, instrucciones[i]);
    }

    list_add(lista_instrucciones, hilo_instrucciones);
}

uint32_t leer_memoria(uint32_t direccion_fisica) {
    // Verificar que la dirección física esté dentro de los límites de la memoria de usuario
    if (direccion_fisica + sizeof(uint32_t) > TAM_MEMORIA) {
        log_error(LOGGER_MEMORIA, "Error de segmentación: Dirección física %d fuera de los límites de memoria de usuario", direccion_fisica);
        return 0;  // Retornar 0 o algún valor de error para indicar fallo de segmentación
    }

    // Leer los primeros 4 bytes a partir de la dirección física
    uint32_t valor = *((uint32_t*)(direccion_fisica));

    // Log de éxito
    log_info(LOGGER_MEMORIA, "Lectura exitosa: Dirección física %d, Valor %d", direccion_fisica, valor);

    return valor;
}

void escribir_memoria(uint32_t direccion_fisica, uint32_t valor) {
    // Verificar que la dirección física esté dentro de los límites de la memoria de usuario
    if (direccion_fisica + sizeof(uint32_t) > TAM_MEMORIA) {
        log_error(LOGGER_MEMORIA, "Error de segmentación: Dirección física %d fuera de los límites de memoria de usuario", direccion_fisica);
        return;
    }

    // Escribir los 4 bytes de `valor` a partir de `direccion_fisica`
    *((uint32_t*)(direccion_fisica)) = valor;

    // Log de éxito y respuesta
    log_info(LOGGER_MEMORIA, "Escritura exitosa: Dirección física %d, Valor %d", direccion_fisica, valor);
    enviar_respuesta(socket_memoria_cpu_dispatch, "OK");  // Responder OK al cliente
}