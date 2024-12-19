#include "include/gestor.h"


t_list* lista_procesos; // TIPO t_proceso_memoria
t_list* lista_instrucciones; // TIPO t_hilo_instrucciones

pthread_mutex_t mutex_procesos;
pthread_mutex_t mutex_instrucciones;

sem_t sem_respuesta;

void inicializar_datos(){
    lista_procesos = list_create();
    lista_instrucciones = list_create();

    pthread_mutex_init(&mutex_procesos, NULL);
    pthread_mutex_init(&mutex_instrucciones, NULL);

    sem_init(&sem_respuesta, 0, 0);
}


//---------------------------------------|
// CONEXION KERNEL DE CREACION DE PROCESO|
//---------------------------------------|
t_proceso_memoria* recibir_proceso(int socket) {
    t_paquete* paquete = recibir_paquete(socket);
    t_proceso_memoria* proceso = deserializar_proceso(paquete->buffer);
    eliminar_paquete(paquete);
    return proceso;
}


t_proceso_memoria* deserializar_proceso(t_buffer* buffer) {
    t_proceso_memoria* proceso = malloc(sizeof(t_proceso_memoria));
    if(proceso == NULL) {
        printf("Error al asignar memoria al proceso");
        return NULL;
    }
    void* stream = buffer->stream;
    int desplazamiento = 0;


    memcpy(&(proceso->pid), stream + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);


    memcpy(&(proceso->limite), stream + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);


    proceso->contexto = malloc(sizeof(t_registros));
    if (!proceso->contexto) {
        free(proceso);
        log_error(LOGGER_MEMORIA, "Error al asignar memoria para el contexto.");
        return NULL;
    }


    memcpy(proceso->contexto, buffer->stream + desplazamiento, sizeof(t_registros));


    return proceso;
}


void eliminar_proceso_de_lista(uint32_t pid) {
    for (int i = 0; i < list_size(lista_procesos); i++) {
        t_proceso_memoria* proceso_actual = list_get(lista_procesos, i);
        if (pid == proceso_actual->pid) {
            free(proceso_actual->contexto);
            free(proceso_actual);
            list_remove(lista_procesos, i);
            break;
        }
    }
}


//-----------------|
// CREACION DE HILO|
//-----------------|
t_hilo_memoria* recibir_hilo_memoria(int socket){
    t_paquete* paquete = recibir_paquete(socket);
    t_hilo_memoria* hilo = deserializar_hilo_memoria(paquete->buffer);
    eliminar_paquete(paquete);
    return hilo;
}


t_hilo_memoria* deserializar_hilo_memoria(t_buffer* buffer) {
    t_hilo_memoria* hilo = malloc(sizeof(t_hilo_memoria));
    if(!hilo) {
        printf("Error al crear el hilo recibido");
        return NULL;
    }


    void* stream = buffer->stream;
    int desplazamiento = 0;
    uint32_t tamanio_archivo;


    memcpy(&(hilo->tid), stream + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);


    memcpy(&(hilo->pid_padre), stream + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);


    memcpy(&(tamanio_archivo), stream + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);


    hilo->archivo = malloc(tamanio_archivo);
    if(hilo->archivo == NULL) {
        printf("Error al deserializar el tamaño del archivo");
        return NULL;
    }


    memcpy(hilo->archivo, stream + desplazamiento, tamanio_archivo);

    return hilo;
}


void agregar_instrucciones_a_lista(uint32_t pid, uint32_t tid, char* archivo) {
    FILE *file = fopen(archivo, "r");
    if (file == NULL) {
        log_error(LOGGER_MEMORIA, "Error al abrir el archivo de pseudocódigo: %s", archivo);
        return;
    }

    char line[100];
    t_list* lst_instrucciones = list_create();
    while (fgets(line, sizeof(line), file) != NULL) {

        t_instruccion* inst = malloc(sizeof(t_instruccion));

        inst->nombre = calloc(1, 100);
        inst->parametro1 = calloc(1, 100);
        inst->parametro2 = calloc(1, 100);
        inst->parametro3 = -1;

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
    }

    fclose(file);

    t_hilo_instrucciones* instrucciones_hilo = malloc(sizeof(t_hilo_instrucciones));
    instrucciones_hilo->tid = tid;
    instrucciones_hilo->pid = pid;
    instrucciones_hilo->instrucciones = lst_instrucciones;

    list_add(lista_instrucciones, instrucciones_hilo);
}


//--------------------|
//FINALIZACION DE HILO|
//--------------------|
void eliminar_espacio_hilo(t_hilo_memoria* hilo) {
    eliminar_instrucciones_hilo(hilo->pid_padre,hilo->tid);
    log_info(LOGGER_MEMORIA, "Hilo finalizado - PID: %d, TID: %d", hilo->pid_padre, hilo->tid);
    free(hilo->archivo);
    free(hilo);
}


t_registros* obtener_contexto(uint32_t pid) {
    for(int i = 0; i < list_size(lista_procesos); i++) {
        t_proceso_memoria* pcb_actual = list_get(lista_procesos, i);
        if(pcb_actual->pid == pid) {
            return pcb_actual->contexto;
        }
    }
    return NULL;
}


void eliminar_instrucciones_hilo(uint32_t pid, uint32_t tid) {
    // lista_instrucciones es de tipo t_hilo_instrucciones*
    for(int i = 0; i < list_size(lista_instrucciones); i++) {
        t_hilo_instrucciones* instruccion_actual = list_get(lista_instrucciones, i);
        if(instruccion_actual->tid == tid && instruccion_actual->pid == pid) {
            list_remove(lista_instrucciones, i);
            liberar_instrucciones(instruccion_actual->instrucciones);
            free(instruccion_actual);
            return;
        }
    }
}


void liberar_instrucciones(t_list* instrucciones) {
    for(int i=0; i < list_size(instrucciones); i++) {
        t_instruccion* inst = list_get(instrucciones, i);
        free(inst->nombre);
        free(inst->parametro1);
        free(inst->parametro2);
        free(inst);
    }
}


//-----------|
//MEMORY DUMP|
//-----------|
t_pid_tid* recibir_identificadores(int socket) {
    t_paquete* paquete = recibir_paquete(socket);
    t_pid_tid* ident = deserializar_identificadores(paquete->buffer);
    eliminar_paquete(paquete);
    return ident;
}


t_pid_tid* deserializar_identificadores(t_buffer* buffer) {
    t_pid_tid* ident = malloc(sizeof(t_pid_tid));
    if(ident == NULL) {
        printf("Error al asignar memoria");
        return NULL;
    }
   
   void * stream = buffer->stream;
   int desplazamiento = 0;
   
   memcpy(&(ident->pid), stream + desplazamiento, sizeof(uint32_t));
   desplazamiento += sizeof(uint32_t);


   memcpy(&(ident->tid), stream + desplazamiento, sizeof(uint32_t));
   
   return ident;
}

int solicitar_archivo_filesystem(uint32_t pid, uint32_t tid) {
    char nombre_archivo[256];
    time_t t = time(NULL);
    struct tm* tiempo = localtime(&t);
    sprintf(nombre_archivo, "<%d>-<%d>-<%d>", pid, tid, tiempo->tm_sec);

    char* contenido_proceso = obtener_contenido_proceso(pid, tid);
    int tamanio = strlen(contenido_proceso);
    log_warning(LOGGER_MEMORIA, "EL TAMANIO DEL CONTENIDO ES DE %d", tamanio);
    int resultado = mandar_solicitud_dump_memory(nombre_archivo, contenido_proceso, tamanio);


    // Liberar memoria si es necesario
    //free(contenido_proceso); DESCOMENTAR DESPUES :P

    return resultado;
}

char* obtener_contenido_proceso(uint32_t pid, uint32_t tid) {
    t_proceso_memoria* pcb = obtener_proceso_memoria(pid);
    if (!pcb || !pcb->contexto) {
        log_error(LOGGER_MEMORIA, "Proceso o contexto no encontrado para PID: %d", pid);
        return NULL;
    }

    t_registros* registros = pcb->contexto;

    const char* nombres_registros[] = {"AX:", "BX:", "CX:", "DX:", "EX:", "FX:", "GX:", "HX:"};
    size_t tamanio_total = 30 + (8 * 16); // 30 bytes para el encabezado y 16 bytes por registro.

    // Reservar memoria para el contenido
    char* contenido = malloc(tamanio_total + 1);
    if (!contenido) {
        log_error(LOGGER_MEMORIA, "Error al asignar memoria para contenido del proceso.");
        return NULL;
    }
    contenido[0] = '\0'; // Inicializar el string.

    // Agregar encabezado "<PID> <TID>"
    snprintf(contenido, tamanio_total, "<%d> <%d>\n", pid, tid);

    // Agregar los registros al contenido
    for (int i = 0; i < 8; i++) {
        char linea[20]; // Espacio suficiente para "REGISTRO: valor\n"
        snprintf(linea, sizeof(linea), "%s %u\n", nombres_registros[i], ((uint32_t*)registros)[i]);
        strcat(contenido, linea);
    }

    return contenido;
}

t_proceso_memoria* obtener_proceso_memoria(uint32_t pid) {
    for(int i=0; i < list_size(lista_procesos); i++) {
        t_proceso_memoria* pcb = list_get(lista_procesos, i);
        if(pcb->pid == pid) {
            return pcb;
        }
    }
    return NULL;
}


t_list* convertir_registros_a_char(t_registros* registros) {
    t_list* lista = list_create();
    if (!lista) return NULL;


    uint32_t lista_registros[] = {
        registros->AX, registros->BX, registros->CX, registros->DX,
        registros->EX, registros->FX, registros->GX, registros->HX
    };


    for (int i = 0; i < 8; i++) {
        char* registro_str = malloc(sizeof(uint32_t) + 1); // Espacio para un entero de 32 bits (10 dígitos máx.) + nulo
        if (!registro_str) {
            list_destroy_and_destroy_elements(lista, free); // Limpiar si hay error
            return NULL;
        }
        sprintf(registro_str, "%d", lista_registros[i]);
        list_add(lista, registro_str);
    }


    return lista;
}


t_list* obtener_lista_instrucciones_por_tid(uint32_t pid, uint32_t tid) {
    for (int i = 0; i < list_size(lista_instrucciones); i++) {
        t_hilo_instrucciones* hilo_instrucciones = list_get(lista_instrucciones, i);
        if (hilo_instrucciones->tid == tid && hilo_instrucciones->pid == pid) {
            return hilo_instrucciones->instrucciones;
        }
    }


    return NULL;
}


// ------------------|
// INSTRUCCIONES CPU |
// ------------------|
t_actualizar_contexto* recibir_actualizacion(int socket) {
    t_paquete* paquete = recibir_paquete(socket);
    t_actualizar_contexto* act_cont = deserializar_actualizacion(paquete->buffer);
    eliminar_paquete(paquete);
    return act_cont;
}


t_actualizar_contexto* deserializar_actualizacion(t_buffer* buffer) {
    t_actualizar_contexto* act_cont = malloc(sizeof(t_actualizar_contexto));
    if(!act_cont) {
        printf("Error al crear el act_cont");
        return NULL;
    }

    void* stream = buffer->stream;
    int desplazamiento = 0;

    memcpy(&(act_cont->pid), stream + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);

    memcpy(&(act_cont->tid), stream + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);

    act_cont->contexto = malloc(sizeof(t_registros));
    if(!act_cont->contexto) {
        free(act_cont);
        log_error(LOGGER_MEMORIA, "Error al asignar espacio a los registros");
        return NULL;
    }

    memcpy(act_cont->contexto, stream + desplazamiento, sizeof(t_registros));

    // memcpy(&(act_cont->contexto->AX), stream + desplazamiento, sizeof(uint32_t));
    // desplazamiento += sizeof(uint32_t);

    // memcpy(&(act_cont->contexto->BX), stream + desplazamiento, sizeof(uint32_t));
    // desplazamiento += sizeof(uint32_t);

    // memcpy(&(act_cont->contexto->CX), stream + desplazamiento, sizeof(uint32_t));
    // desplazamiento += sizeof(uint32_t);

    // memcpy(&(act_cont->contexto->DX), stream + desplazamiento, sizeof(uint32_t));
    // desplazamiento += sizeof(uint32_t);

    // memcpy(&(act_cont->contexto->EX), stream + desplazamiento, sizeof(uint32_t));
    // desplazamiento += sizeof(uint32_t);

    // memcpy(&(act_cont->contexto->FX), stream + desplazamiento, sizeof(uint32_t));
    // desplazamiento += sizeof(uint32_t);

    // memcpy(&(act_cont->contexto->GX), stream + desplazamiento, sizeof(uint32_t));
    // desplazamiento += sizeof(uint32_t);

    // memcpy(&(act_cont->contexto->HX), stream + desplazamiento, sizeof(uint32_t));

    return act_cont;
}

int enviar_instruccion(int socket, t_instruccion* inst) {
    t_paquete* paquete = crear_paquete_con_codigo_de_operacion(INSTRUCCION);
    uint32_t tam_nom = strlen(inst->nombre) + 1;
    uint32_t tam_p1 = strlen(inst->parametro1) + 1;
    uint32_t tam_p2 = strlen(inst->parametro2) + 1;

    paquete->buffer->size = sizeof(uint32_t) * 3 + tam_nom + tam_p1 + tam_p2 + sizeof(int);
    paquete->buffer->stream = malloc(paquete->buffer->size);

    int desplazamiento = 0;

    memcpy(paquete->buffer->stream + desplazamiento, &tam_nom,sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(paquete->buffer->stream + desplazamiento, inst->nombre, tam_nom);
    desplazamiento += tam_nom;
   
    memcpy(paquete->buffer->stream + desplazamiento, &tam_p1,sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(paquete->buffer->stream + desplazamiento, inst->parametro1, tam_p1);
    desplazamiento += tam_p1;

    memcpy(paquete->buffer->stream + desplazamiento, &tam_p2,sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(paquete->buffer->stream + desplazamiento, inst->parametro2, tam_p2);
    desplazamiento += tam_p2;

    memcpy(paquete->buffer->stream + desplazamiento, &(inst->parametro3), sizeof(int));
   
    int resultado = enviar_paquete(paquete, socket);
    eliminar_paquete(paquete);

    return resultado;
}


void enviar_valor_leido_cpu(int socket, uint32_t dire_fisica, uint32_t valor) {
    t_paquete* paquete = crear_paquete_con_codigo_de_operacion(PEDIDO_READ_MEM);

    paquete->buffer->size = sizeof(uint32_t) * 2;
    paquete->buffer->stream = malloc(paquete->buffer->size);


    int desplazamiento = 0;


    memcpy(paquete->buffer->stream + desplazamiento, &(dire_fisica), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(paquete->buffer->stream + desplazamiento, &(valor), sizeof(uint32_t));
   
   
    if(enviar_paquete(paquete, socket) == 0) {
        log_info(LOGGER_MEMORIA, "Valor enviado a Cpu");
    } else {
        log_error(LOGGER_MEMORIA, "Error al enviar el valor");
    }
    eliminar_paquete(paquete);
}


// Lista de instrucciones es de tipo t_proceso_instruccion
t_instruccion* obtener_instruccion(uint32_t pid, uint32_t tid, uint32_t pc) {
    for(int i = 0; i < list_size(lista_instrucciones); i++) {
        t_hilo_instrucciones* instruccion_actual = list_get(lista_instrucciones, i);
        if(instruccion_actual->tid == tid && instruccion_actual->pid == pid) {
            return (list_get(instruccion_actual->instrucciones, pc));
        }
    }
    return NULL;
}

void procesar_solicitud_contexto(int socket_cliente, uint32_t pid, uint32_t tid) {
    for (int i = 0; i < list_size(lista_procesos); i++) {
        t_proceso_memoria* proceso = list_get(lista_procesos, i);
       
        if (proceso->pid == pid) {
            enviar_contexto_cpu(socket_cliente, proceso);
            log_info(LOGGER_MEMORIA, "## Contexto <Solicitado> - (<%d>:<%d>)", pid, tid);
            return;
        }
    }

    log_error(LOGGER_MEMORIA, "No se encontró contexto para el PID: %d", pid);
    enviar_mensaje("ERROR", socket_memoria_cpu);
}


void enviar_contexto_cpu(int socket, t_proceso_memoria* proceso) {
    t_paquete* paquete = crear_paquete_con_codigo_de_operacion(CONTEXTO);
   
    paquete->buffer->size = sizeof(uint32_t) + sizeof(t_registros);
    paquete->buffer->stream = malloc(paquete->buffer->size);

    int desplazamiento = 0;

    memcpy(paquete->buffer->stream + desplazamiento, &(proceso->pid), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(paquete->buffer->stream + desplazamiento, proceso->contexto, sizeof(t_registros));
   
    enviar_paquete(paquete, socket);
    eliminar_paquete(paquete);
}


void procesar_actualizacion_contexto(int socket_cliente, uint32_t pid, uint32_t tid, t_registros* nuevo_contexto) {
    for (int i = 0; i < list_size(lista_procesos); i++) {
        t_proceso_memoria* proceso = list_get(lista_procesos, i);
        if (proceso->pid == pid) {
            memcpy(proceso->contexto, nuevo_contexto, sizeof(t_registros));
            log_info(LOGGER_MEMORIA, "## Contexto <Actualizado> - (<%d>:<%d>)", pid, tid);
            enviar_mensaje("OK", socket_cliente);
            return;
        }
    }

    log_error(LOGGER_MEMORIA, "No se encontró contexto para actualizar en PID: %d, TID: %d", pid, tid);
    enviar_mensaje("ERROR", socket_cliente);
}


int mandar_solicitud_dump_memory(char* nombre_archivo, char* contenido_proceso, uint32_t tamanio) {
    t_paquete* paquete = crear_paquete_con_codigo_de_operacion(DUMP_MEMORY);
    uint32_t tamanio_nombre = strlen(nombre_archivo) + 1;
    
    paquete->buffer->size = sizeof(uint32_t) * 3 + tamanio_nombre;
    paquete->buffer->stream = malloc(paquete->buffer->size);

    int desplazamiento = 0;

    memcpy(paquete->buffer->stream + desplazamiento, &(tamanio_nombre), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(paquete->buffer->stream + desplazamiento, nombre_archivo, tamanio_nombre);
    desplazamiento += tamanio_nombre;
    memcpy(paquete->buffer->stream + desplazamiento, &(tamanio), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(paquete->buffer->stream + desplazamiento, contenido_proceso, tamanio);
   
    int resultado = enviar_paquete(paquete, socket_memoria_filesystem);
    eliminar_paquete(paquete);
   
    return resultado;
}

// =========|
// WRITE MEM|
// =========|
t_write_mem* recibir_write_mem(int socket) {
    t_paquete* paquete = recibir_paquete(socket);
    t_write_mem* wri_mem = deserializar_write_mem(paquete->buffer);
    eliminar_paquete(paquete);
    return wri_mem;
}


t_write_mem* deserializar_write_mem(t_buffer* buffer) {
    t_write_mem* wri_mem = malloc(sizeof(t_write_mem));
    if(wri_mem == NULL) {
        printf("Error al asignar memoria");
        return NULL;
    }


    void * stream = buffer->stream;
    int desplazamiento = 0;
   
    memcpy(&(wri_mem->pid), stream + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);


    memcpy(&(wri_mem->tid), stream + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);


    memcpy(&(wri_mem->dire_fisica_wm), stream + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
   
    memcpy(&(wri_mem->valor_escribido), stream + desplazamiento, sizeof(uint32_t));
   
    return wri_mem;
}


int escribir_memoria(uint32_t direccion_fisica, uint32_t valor) {
    // Verificar que la dirección física esté dentro de los límites de la memoria de usuario
    if (direccion_fisica + sizeof(uint32_t) > TAM_MEMORIA) {
        log_error(LOGGER_MEMORIA, "Error de segmentación: Dirección física %d fuera de los límites de memoria de usuario", direccion_fisica);
        return -1;
    }
    
    memcpy((uint8_t*)memoria_usuario + direccion_fisica, &valor, sizeof(uint32_t));

    uint32_t valor_leido;
    memcpy(&valor_leido, (uint8_t*) memoria_usuario + direccion_fisica, sizeof(uint32_t));
    log_warning(LOGGER_MEMORIA, "SE ESCRIBIO %d, EN %d", valor_leido, direccion_fisica);
   
    return 1;
}


// ========|
// READ_MEM|
// ========|


t_read_mem* recibir_read_mem(int socket) {
    t_paquete* paquete = recibir_paquete(socket);
    t_read_mem* read_mem = deserializar_read_mem(paquete->buffer);
    eliminar_paquete(paquete);
    return read_mem;
}


t_read_mem* deserializar_read_mem(t_buffer* buffer) {
    t_read_mem* read_mem = malloc(sizeof(t_read_mem));
    void* stream = buffer->stream;
    int desplazamiento = 0;


    memcpy(&read_mem->pid, stream + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(&read_mem->tid, stream + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(&read_mem->direccion_fisica, stream + desplazamiento, sizeof(uint32_t));
   
    return read_mem;
}


uint32_t leer_memoria(uint32_t direccion_fisica) {
    // Verificar que la dirección física esté dentro de los límites de la memoria de usuario
    if (direccion_fisica + sizeof(uint32_t) > TAM_MEMORIA) {
        log_error(LOGGER_MEMORIA, "Error de segmentación: Dirección física %d fuera de los límites de memoria de usuario", direccion_fisica);
        return 0;  // Retornar 0 o algún valor de error para indicar fallo de segmentación
    }

    uint32_t valor;
    memcpy(&valor, (uint8_t*) memoria_usuario + direccion_fisica, sizeof(uint32_t));

    log_warning(LOGGER_MEMORIA, "SE LEYO EL VALOR %d", valor);

    return valor;
}


// ==================|
// PEDIDO_INSTRUCCION|
// ==================|
t_pedido_instruccion* recibir_pedido_instruccion(int socket) {
    t_paquete* paquete = recibir_paquete(socket);
    t_pedido_instruccion* ped_inst = deserializar_pedido_instruccion(paquete->buffer);
    eliminar_paquete(paquete);
    return ped_inst;
}


t_pedido_instruccion* deserializar_pedido_instruccion(t_buffer* buffer) {
    t_pedido_instruccion *ped_inst = malloc(sizeof(t_pedido_instruccion));
   
    if(ped_inst == NULL) {
        printf("Error al asignar memoria");
        return NULL;
    }
   
    void * stream = buffer->stream;
    int desplazamiento = 0;
   
    memcpy(&(ped_inst->pid), stream + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
   
    memcpy(&(ped_inst->tid), stream + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
   
    memcpy(&(ped_inst->pc), stream + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
   
    return ped_inst;
}


// =============================|
// BASE_MEMORIA y LIMITE_MEMORIA|
// =============================|
int enviar_valor_uint_cpu(int socket, uint32_t valor, op_code codigo) {
    t_paquete* paquete = crear_paquete_con_codigo_de_operacion(codigo);
     
    paquete->buffer->size = sizeof(uint32_t);
    paquete->buffer->stream = malloc(paquete->buffer->size);    

    memcpy(paquete->buffer->stream , &valor, sizeof(uint32_t));

    int resultado = enviar_paquete(paquete, socket);
    if(resultado==-1){
        log_error(LOGGER_MEMORIA, "Error al enviar el valor a la CPU");
    }
    
    eliminar_paquete(paquete);
    return resultado;
}

