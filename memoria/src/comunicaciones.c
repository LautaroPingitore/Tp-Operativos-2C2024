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
            asignar_espacio_memoria(proceso_nuevo);
            log_info(logger, "Proceso Creado - PID: %d - Tamanio: %d", proceso_nuevo->pid, proceso_nuevo->limite);
            break;

        case PROCESS_EXIT:
            t_proceso_memoria* proceso_a_eliminar = recibir_proceso_kernel(cliente_socket);
            liberar_espacio_memoria(proceso_a_eliminar);
            log_info(logger, "Proceso Destruido - PID: %d - Tamanio %d", proceso_a_eliminar->pid, proceso_nuevo->limite);
            break;

        case DUMP_MEMORY:
            t_proceso_memoria* proceso = recibir_proceso_kernel(cliente_socket);
            // SI HACEMOS QUE CADA HILO TENGA ASIGNADO EL PID DEL PROCESO QUE LO CREO SE PODRIA ARREGLAR ESTO
            // AUNQUE HAY QUE MODIFICAR MUCHO EN KERNEL
            solicitar_archivo_filesystem(proceso->pid, proceso->tid);//, timestamp???); // dump memory todavia no la hicimos
            log_info(logger, "No implementado. Respondiendo OK.");
            break;

        case THREAD_CREATE:
        
            log_info(logger, "No implementado. Respondiendo OK.");
            break;

        case THREAD_EXIT:
        
            log_info(logger, "No implementado. Respondiendo OK.");
            break;
        case HANDSHAKE_cpu:
            recibir_mensaje(cliente_socket, logger);
            log_info(logger, "Este deberia ser el canal mediante el cual nos comunicamos con la CPU");
            break;

        case PEDIDO_INSTRUCCION:
            uint32_t pid, pc;
            recibir_pedido_instruccion(&pid, &pc, cliente_socket);
            t_instruccion *instruccion = obtener_instruccion(pid, pc);
            if (instruccion != NULL)
            {
                enviar_instruccion(cliente_socket, instruccion);
                log_info(logger, "Se envió la instrucción al PID %d, PC %d", pid, pc);
            }
            else
            {
                log_error(logger, "No se encontró la instrucción para PID %d, PC %d", pid, pc);
            }
            break;

        // Peticiones stub (sin hacer nada)
        case PEDIDO_SET:{
            uint32_t pid, registro, valor;
            recibir_set(&pid, &registro, &valor, cliente_socket);
            t_contexto_ejecucion *contexto = obtener_contexto(pid);

            t_registros *registros = contexto->registros;
            switch (registro) {
                case 0: registros->AX = valor; break;
                case 1: registros->BX = valor; break;
                case 2: registros->CX = valor; break;
                case 3: registros->DX = valor; break;
                case 4: registros->EX = valor; break;
                case 5: registros->FX = valor; break;
                case 6: registros->GX = valor; break;
                case 7: registros->HX = valor; break;
                default: enviar_respuesta(cliente_socket, "ERROR: Registro no válido"); continue;
                }
            enviar_respuesta(cliente_socket, "OK");
            log_info(logger, "Seteado registro %d a valor %d para PID %d", registro, valor, pid);
            break;
        }
        case PEDIDO_READ_MEM: {
                uint32_t pid, direccion_logica;
                recibir_read_mem(&pid, &direccion_logica, cliente_socket);

                uint32_t valor = *(uint32_t *)((char *)memoriaUsuario + direccion_logica);

                enviar_respuesta(cliente_socket, (char *)&valor); // Enviamos el valor leído
                log_info(logger, "Se leyó memoria en dirección %d para PID %d", direccion_logica, pid);
            break;
        }
        case PEDIDO_WRITE_MEM: {
                uint32_t pid, direccion_logica, valor;
                recibir_write_mem(&pid, &direccion_logica, &valor, cliente_socket);
                *(uint32_t *)((char *)memoriaUsuario + direccion_logica) = valor;
                enviar_respuesta(cliente_socket, "OK");
                log_info(logger, "Se escribió valor %d en dirección %d para PID %d", valor, direccion_logica, pid);
                break;
        }
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
t_list* lista_procesos_memoria;

// PUEDE ESTAR METIDO EN EL INICILIZAR LISTA DE PARTICIONES
void inicializar_listas() {
    lista_procesos_memoria = list_create();
}

// RECIBE EL PROCESO DE KERNEL, LO AGREGA A LA LISTA DE PROCESOS DE MEMORIA Y LE ASIGNA ESPACIO
t_proceso_memoria* crear_proceso_en_memoria(int socket_cliente) {
    t_pcb* pcb = recibir_proceso_kernel(socket_cliente);
    int resultado = asignar_espacio_memoria(pcb_memoria, ALGORITMO_BUSQUEDA);
    if(resultado) {
        t_proceso_memoria pcb_memoria;
        pcb_memoria->pid = pcb->PID;
        pcb_memoria->limite = pcb->LIMITE;
        pcb_memoria->contexto = pcb->CONTEXTO
        agregar_a_lista_procesos(proceso_memoria);
        respuesta_asignacion_kernel(socket_cliente, resultado);//okey);
        log_info(LOGGER_MEMORIA, "## Proceso Creado - PID: %d - Tamaño: %d", proceso_memoria->pid, proceso_memoria->limite);
        return pcb_memoria;
    } else {
        respuesta_asignacion_kernel(socket_cliente, resultado);//mal);
        return NULL;
    }

}

// RECIBE EL PROCESO DE KERNEL (T_PCB)
t_pcb* recibir_proceso_kernel(int cliente_socket) {
    t_pcb* proceso = malloc(sizeof(t_pcb));
    recv(cliente_socket, &(proceso->PID), sizeof(uint32_t), 0);
    recv(cliente_socket, &(proceso->TAMANIO), sizeof(uint32_t), 0);
    recv(cliente_socket, &(proceso->CONTEXTO), sizeof(t_contexto_ejecucion), 0);
    proceso->TIDS = NULL;
    proceso->ESTADO = NEW;
    proceso->MUTEXS = NULL;
    proceso->CANTIDAD_RECURSOS = 0;
    return proceso;
}

// LISTA DE PROCESOS QUE ESTAN EN MEMORIA
void agregar_a_lista_procesos(t_proceso_memoria* proceso) {
    proceso = malloc(sizeof(t_proceso_memoria));
    list_add(lista_procesos_memoria, proceso);
}

// PODEMOS USAR EL ENVIAR_RESPUESTA(CLIENTE_SOCKET, "OK") O ENVIAR_RESPUESTA(CLIENTE_SOCKET, "ERROR")
void respuesta_asignacion_kernel(int socket_cliente, int resultado) {
    // Enviar resultado al cliente (1 si la asignación fue exitosa, -1 si falló)
    if (send(socket_cliente, &resultado, sizeof(int), 0) <= 0) {
        log_error(LOGGER_MEMORIA, "Error al enviar la respuesta de asignación de memoria.");
    }
}


//-------------------------------------------|
// CONEXION KERNEL DE FINALIZACION DE PROCESO|
//-------------------------------------------|

void eliminar_proceso_recibido(int socket_cliente) {
    t_pcb* proceso = recibir_proceso_kernel(socket_cliente);
    t_proceso_memoria* proceso_memoria = eliminar_proceso_de_lista(proceso->PID);
    if(proceso_memoria == NULL) {
        log_error(LOGGER_MEMORIA, "## Proceso PID: %d .No encontrado en memoria", proceso->PID);
    }
    liberar_espacio_memoria(proceso_memoria);
    log_info(LOGGER_MEMORIA, "## Proceso Destruido - PID: %d - Tamaño: %d", proceso_memoria->pid, proceso_memoria->limite);
}

t_proceso_memoria* eliminar_proceso_de_lista(uint32_t pid) {
    for (int i = 0; i < list_size(lista_procesos_memoria); i++) {
        t_proceso_memoria* proceso_actual = list_get(lista_procesos_memoria, i);
        if (pid == proceso_actual->pid) {
            t_proceso_memoria* proceso_eliminado = list_remove(lista_procesos_memoria, i);
            return proceso_eliminado;
        }
    }
    return NULL;
}

//-----------------|
// CREACION DE HILO|
//-----------------|

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
        log_info(LOGGER_MEMORIA, "Hilo finalizado - PID: %d, TID: %d", pid, tid);
        enviar_respuesta(cliente_socket, "OK");
    } else {
        log_error(LOGGER_MEMORIA, "Error al finalizar el hilo - PID: %d, TID: %d", pid, tid);
        enviar_respuesta(cliente_socket, "ERROR");
    }
}



//-----------|
//MEMORY DUMP|
//-----------|
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


void enviar_instruccion(int cliente_socket, t_instruccion* instruccion){
    // Enviar el nombre de la instrucción
    send(cliente_socket, &instruccion->nombre, sizeof(nombre_instruccion), 0);
    // Enviar cada parámetro
    uint32_t tam_param1 = strlen(instruccion->parametro1) + 1;
    uint32_t tam_param2 = strlen(instruccion->parametro2) + 1;

    send(cliente_socket, &tam_param1, sizeof(uint32_t), 0);
    send(cliente_socket, instruccion->parametro1, tam_param1, 0);
    send(cliente_socket, &tam_param2, sizeof(uint32_t), 0);
    send(cliente_socket, instruccion->parametro2, tam_param2, 0);
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

void recibir_read_mem(uint32_t* pid, uint32_t* direccion_logica, int cliente_socket){
    recv(cliente_socket, pid, sizeof(uint32_t), 0);
    recv(cliente_socket, direccion_logica, sizeof(uint32_t), 0);
}

void recibir_write_mem(uint32_t* pid, uint32_t* direccion_logica, uint32_t* valor, int cliente_socket){
    recv(cliente_socket, pid, sizeof(uint32_t), 0);
    recv(cliente_socket, direccion_logica, sizeof(uint32_t), 0);
    recv(cliente_socket, valor, sizeof(uint32_t), 0);
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
    cantidad_procesos++;
    lista_contextos = realloc(lista_contextos, cantidad_procesos * sizeof(t_proceso_contexto));
    lista_contextos[cantidad_procesos - 1].pid = pid;
    lista_contextos[cantidad_procesos - 1].contexto = *nuevo_contexto;
}

void agregar_instrucciones(uint32_t pid, t_instruccion** instrucciones, size_t cantidad) {
    lista_instrucciones = realloc(lista_instrucciones, cantidad_procesos * sizeof(t_proceso_instrucciones));
    lista_instrucciones[cantidad_procesos - 1].pid = pid;
    lista_instrucciones[cantidad_procesos - 1].instrucciones = instrucciones;
    lista_instrucciones[cantidad_procesos - 1].cantidad_instrucciones = cantidad;
}