#include "include/gestor.h"

t_list* lista_procesos; // TIPO t_proceso_memoria
t_list* lista_instrucciones; // TIPO t_hilo_instrucciones

pthread_mutex_t mutex_procesos;
pthread_mutex_t mutex_instrucciones;

void inicializar_datos(){
    pthread_mutex_lock(&mutex_procesos);
    lista_procesos = list_create();
    pthread_mutex_unlock(&mutex_procesos);

    pthread_mutex_lock(&mutex_instrucciones);
    lista_instrucciones = list_create();
    pthread_mutex_unlock(&mutex_instrucciones);

    pthread_mutex_init(&mutex_procesos, NULL);
    pthread_mutex_init(&mutex_instrucciones, NULL);
}

void* procesar_conexion_memoria(void *void_args){
    t_procesar_conexion_args *args = (t_procesar_conexion_args *)void_args;
    t_log *logger = args->log;
    int cliente_socket = args->fd;
    char *server_name = args->server_name;
    free(args);

    op_code cod;
    while (1) {
        // Recibir código de operación
        ssize_t bytes_recibidos = recv(cliente_socket, &cod, sizeof(op_code), 0);
        if (bytes_recibidos <= 0) { // El cliente cerró la conexión o hubo un error
            log_error(logger, "El cliente cerró la conexión.");
            break; // Salir del bucle y cerrar el hilo
        }

        switch (cod) {
            case HANDSHAKE_kernel: // Simplemente avisa que se conecta a kernel 
                log_info(logger, "## Kernel Conectado - FD del socket: <%d>", cliente_socket);
                break;

            case PROCESS_CREATE:
                t_proceso_memoria* proceso_nuevo = recibir_proceso(cliente_socket);

                if(proceso_nuevo == NULL) {
                    enviar_mensaje("ERROR",cliente_socket);
                    break;
                }
                
                if (asignar_espacio_memoria(proceso_nuevo, ALGORITMO_BUSQUEDA) == -1) {
                    enviar_mensaje("ERROR: No se pudo asignar memoria", cliente_socket);
                    free(proceso_nuevo->contexto);
                    free(proceso_nuevo);
                } else {
                    pthread_mutex_lock(&mutex_procesos);
                    list_add(lista_procesos, proceso_nuevo);
                    pthread_mutex_unlock(&mutex_procesos);

                    enviar_mensaje("OK", cliente_socket);
                    log_info(logger, "## Proceso <Creado> -  PID: <%d> - Tamaño: <%d>", proceso_nuevo->pid, proceso_nuevo->limite);
                }
                break;

            case PROCESS_EXIT: // TERMINADO
                
                t_proceso_memoria* proceso_a_eliminar = recibir_proceso(cliente_socket);

                if (proceso_a_eliminar == NULL) {
                    enviar_mensaje("ERROR", cliente_socket);
                    break;
                }
                
                liberar_espacio_memoria(proceso_a_eliminar);
                pthread_mutex_lock(&mutex_procesos);
                eliminar_proceso_de_lista(proceso_a_eliminar->pid);
                pthread_mutex_unlock(&mutex_procesos);

                log_info(logger, "## Proceso <Destruido> -  PID: <%d> - Tamaño: <%d>", proceso_nuevo->pid, proceso_nuevo->limite);
                enviar_mensaje("OK", cliente_socket);
                free(proceso_a_eliminar->contexto);
                free(proceso_a_eliminar);
                break;

            case THREAD_CREATE:
                t_hilo_memoria* hilo_a_crear = recibir_hilo_memoria(cliente_socket);
                if(!hilo_a_crear) {
                    enviar_mensaje("ERROR", cliente_socket);
                    break;
                }
                
                agregar_instrucciones_a_lista(hilo_a_crear->tid, hilo_a_crear->archivo);
                enviar_mensaje("OK", cliente_socket);
                log_info(logger, "## Hilo <Creado> - (PID:TID) - (<%d>:<%d>)", hilo_a_crear->pid_padre, hilo_a_crear->tid);
                free(hilo_a_crear->archivo);
                free(hilo_a_crear);
                break;

            case THREAD_EXIT:
                t_hilo_memoria* hilo_a_eliminar = recibir_hilo_memoria(cliente_socket);
                if(!hilo_a_eliminar) {
                    enviar_mensaje("ERROR", cliente_socket);
                    break;
                }

                log_info(logger, "## Hilo <Destruido> - (PID:TID) - (<%d>:<%d>)", hilo_a_eliminar->pid_padre, hilo_a_eliminar->tid);

                eliminar_espacio_hilo(hilo_a_eliminar);

                enviar_mensaje("OK", cliente_socket);
                break;

            case DUMP_MEMORY:
                t_pid_tid* ident_dm = recibir_identificadores(cliente_socket);
                
                if(!ident_dm) {
                    enviar_mensaje("ERROR", cliente_socket);
                } 
                if (solicitar_archivo_filesystem(ident_dm->pid, ident_dm->tid) == 0) {
                    log_info(logger, "## Memory Dump solicitado - (PID:TID) - (<%d>:<%d>)",
                             ident_dm->pid, ident_dm->tid);
                    enviar_mensaje("OK", cliente_socket);
                } else {
                    enviar_mensaje("ERROR", cliente_socket);
                }
                free(ident_dm);
                break;

            case HANDSHAKE_cpu: //AVISA QUE SE CONECTO A CPU
                log_info(logger, "## CPU Conectado - FD del socket: <%d>", cliente_socket);
                break;

            case CONTEXTO:
                t_pid_tid* ident_cx = recibir_identificadores(cliente_socket);
                if(!ident_cx) {
                    enviar_mensaje("ERROR", cliente_socket);
                } 
                procesar_solicitud_contexto(cliente_socket, ident_cx->pid, ident_cx->tid);
                free(ident_cx);
                break;

            case ACTUALIZAR_CONTEXTO:
                t_actualizar_contexto* act_cont = recibir_actualizacion(cliente_socket);

                procesar_actualizacion_contexto(cliente_socket, act_cont->pid, act_cont->tid, act_cont->contexto);
                free(act_cont->contexto);
                free(act_cont);
                break;

            case PEDIDO_WRITE_MEM:
                t_write_mem* wri_mem = recibir_write_mem(cliente_socket);
                escribir_memoria(wri_mem->dire_fisica_wm, wri_mem->valor_escribido);
                uint32_t tam_valor_escribido = sizeof(wri_mem->valor_escribido);
                log_info(logger, "## <Escritura> - (PID:TID) - (<%D>:<%d>) - Dir. Física: <%d> - Tamaño: <%d>",
                         wri_mem->pid, wri_mem->tid, wri_mem->dire_fisica_wm, tam_valor_escribido);
                free(wri_mem);
                break;

            case PEDIDO_READ_MEM:
                t_read_mem* read_mem = recibir_read_mem(cliente_socket);
                uint32_t valor_leido = leer_memoria(read_mem->direccion_fisica);
                if (valor_leido != SEGF_FAULT) {
                    enviar_valor_leido_cpu(cliente_socket, read_mem->direccion_fisica, valor_leido);
                    uint32_t tam = sizeof(valor_leido);
                    log_info(LOGGER_MEMORIA, "## <Lectura> - (PID:TID) - (<%d>:<%d>) - Dir. Física: <%d> - Tamaño: <%d>"
                             read_mem->pid, read_mem->tid, tam);
                } else {
                    enviar_mensaje("SEGMENTATION_FAULT", cliente_socket);
                }
                free(read_mem);
                break;

            case PEDIDO_INSTRUCCION:
                t_pedido_instruccion* ped_inst = recibir_pedido_instruccion(cliente_socket);

                t_instruccion* inst = obtener_instruccion(ped_inst->tid, ped_inst->pc);
                if(enviar_instruccion(cliente_socket, inst) == 0) {
                    if(inst->parametro3 == -1) {
                        log_info(logger, "## Obtener instrucción - (PID:TID) - (<%d>:<%d>) - Instrucción: <%s> <%s> <%s>",
                                ped_inst->pid, ped_inst->tid, inst->nombre, inst->parametro1, inst->parametro2);
                    } else {
                        log_info(logger, "## Obtener instrucción - (PID:TID) - (<%d>:<%d>) - Instrucción: <%s> <%s> <%s> <%d>",
                                ped_inst->pid, ped_inst->tid, inst->nombre, inst->parametro1, inst->parametro2, inst->parametro3);
                    }
                } else {
                    enviar_mensaje("NO_INSTRUCCION", cliente_socket);
                }
                free(ped_inst);
                break;

            case SOLICITUD_BASE_MEMORIA:
                uint32_t pid_bm = recibir_pid(cliente_socket);
                t_proceso_memoria* pcb_bm = obtener_proceso_memoria(pid_bm);
                int resultado_bm = enviar_valor_uint_cpu(cliente_socket, pcb_bm->base, SOLICITUD_BASE_MEMORIA);
                if(resultado_bm == 0) {
                    log_info(logger, "Paquete enviado correctamente");
                } else {
                    log_error(logger, "Error al enviar el paquete");
                }
                break;

            case SOLICITUD_LIMITE_MEMORIA:
                uint32_t pid_lm = recibir_pid(cliente_socket);
                t_proceso_memoria* pcb_lm = obtener_proceso_memoria(pid_lm);
                int resultado_lm = enviar_valor_uint_cpu(cliente_socket, pcb_lm->limite, SOLICITUD_LIMITE_MEMORIA);
                if(resultado_lm == 0) {
                    log_info(logger, "Paquete enviado correctamente");
                } else {
                    log_error(logger, "Error al enviar el paquete");
                }
                break;

            case ERROROPCODE:
                log_error(logger, "Cliente desconectado de %s... con cod_op -1", server_name);
                break;

            default:
                log_error(logger, "Codigo desconocido en el servidor de %s, Cod OP: %d", server_name, cod);
                break;
        }
    }


    log_warning(logger, "El cliente se desconectó de %s server", server_name);
    close(cliente_socket);
    return NULL;
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

    proceso->contexto = malloc(sizeof(t_contexto_ejecucion));
    if (!proceso->contexto) {
        free(proceso);
        log_error(LOGGER_MEMORIA, "Error al asignar memoria para el contexto.");
        return NULL;
    }

    memcpy(proceso->contexto, buffer->stream + desplazamiento, sizeof(t_contexto_ejecucion));

    return proceso;
}

void eliminar_proceso_de_lista(uint32_t pid) {
    pthread_mutex_lock(&mutex_procesos);
    for (int i = 0; i < list_size(lista_procesos); i++) {
        t_proceso_memoria* proceso_actual = list_get(lista_procesos, i);
        if (pid == proceso_actual->pid) {
            list_remove_and_destroy_element(lista_procesos, i, (void *)liberar_espacio_memoria);
            break;
        }
    }
    pthread_mutex_unlock(&mutex_procesos);
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

    memcpy(&hilo->tid, stream + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);

    memcpy(&hilo->pid_padre, stream + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);

    memcpy(&tamanio_archivo, stream + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);

    hilo->archivo = malloc(tamanio_archivo);
    if(hilo->archivo == NULL) {
        printf("Error al deserializar el tamaño del archivo");
        return NULL;
    }

    memcpy(&hilo->archivo, stream + desplazamiento, tamanio_archivo);

    return hilo;
}

void agregar_instrucciones_a_lista(uint32_t tid, char* archivo) {
    FILE *file = fopen(archivo, "r");
    if (file == NULL) {
        log_error(LOGGER_MEMORIA, "Error al abrir el archivo de pseudocódigo: %s", archivo);
        return;
    }

    char line[100];
    t_list* lst_instrucciones = list_create();
    while (fgets(line, sizeof(line), file) != NULL) {
        t_instruccion* inst = malloc(sizeof(t_instruccion));

        // WARNING TEMA DEL MALLOC
        inst->nombre = malloc(15);
        inst->parametro1 = malloc(15);
        inst->parametro2 = malloc(15);

        int elementos = sscanf(line, "%s %s %s %d", inst->nombre, inst->parametro1, inst->parametro2, &(inst->parametro3));
        inst->nombre = realloc(inst->nombre, strlen(inst->nombre) + 1);
        inst->parametro1 = realloc(inst->parametro1, strlen(inst->parametro1) + 1);
        inst->parametro2 = realloc(inst->parametro2, strlen(inst->parametro2) + 1);

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
    instrucciones_hilo->instrucciones = lst_instrucciones;

    list_add(lista_instrucciones, instrucciones_hilo);
}

//--------------------|
//FINALIZACION DE HILO|
//--------------------|
void eliminar_espacio_hilo(t_hilo_memoria* hilo) {
    eliminar_instrucciones_hilo(hilo->tid);
    log_info(LOGGER_MEMORIA, "Hilo finalizado - PID: %d, TID: %d", hilo->pid_padre, hilo->tid);
    free(hilo->archivo);
    free(hilo);
}

t_contexto_ejecucion* obtener_contexto(uint32_t pid) {
    for(int i = 0; i < list_size(lista_procesos); i++) {
        t_proceso_memoria* pcb_actual = list_get(lista_procesos, i);
        if(pcb_actual->pid == pid) {
            return pcb_actual->contexto;
        }
    }
    return NULL;
}

void eliminar_instrucciones_hilo(uint32_t tid) {
    // lista_instrucciones es de tipo t_hilo_instrucciones*
    for(int i = 0; i < list_size(lista_instrucciones); i++) {
        t_hilo_instrucciones* instruccion_actual = list_get(lista_instrucciones, i);
        if(instruccion_actual->tid == tid) {
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

    int resultado = mandar_solicitud_dump_memory(nombre_archivo, contenido_proceso, tamanio);

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

char* obtener_contenido_proceso(uint32_t pid, uint32_t tid) {
    // Obtener el proceso por PID
    t_proceso_memoria* pcb = obtener_proceso_memoria(pid);
    if (!pcb || !pcb->contexto) {
        log_error(LOGGER_MEMORIA, "Proceso o contexto no encontrado para PID: %d", pid);
        return NULL;
    }

    // Calcular el tamanio necesario para los registros
    size_t tamanio_total = 0;
    const char* nombres_registros[] = {"AX:", "BX:", "CX:", "DX:", "EX:", "FX:", "GX:", "HX:"};
    tamanio_total += 32; // Espacio para <PID> <TID> (incluye "\n").
    tamanio_total += 8 * 12; // Espacio para 8 registros (aproximado, con valores grandes).

    // Reservar memoria para el contenido
    char* contenido = malloc(tamanio_total + 1);
    if (!contenido) {
        log_error(LOGGER_MEMORIA, "Error al asignar memoria para contenido del proceso.");
        return NULL;
    }
    contenido[0] = '\0'; // Inicializar el string.

    // Agregar encabezado <PID> <TID>
    snprintf(contenido, tamanio_total, "<%d> <%d>\n", pid, tid);

    // Agregar los registros al contenido
    t_list* lista_registros = convertir_registros_a_char(pcb->contexto->registros);
    if (!lista_registros) {
        free(contenido);
        log_error(LOGGER_MEMORIA, "Error al convertir registros para PID: %d", pid);
        return NULL;
    }
    for (int i = 0; i < 8; i++) {
        char linea[50];
        snprintf(linea, sizeof(linea), "%s%s\n", nombres_registros[i], (char*)list_get(lista_registros, i));
        strcat(contenido, linea);
    }

    // Liberar lista de registros
    list_destroy_and_destroy_elements(lista_registros, free);

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
        char* registro_str = malloc(strlen(registro_str) + 1); // Espacio para un entero de 32 bits (10 dígitos máx.) + nulo
        if (!registro_str) {
            list_destroy_and_destroy_elements(lista, free); // Limpiar si hay error
            return NULL;
        }
        sprintf(registro_str, "%d", lista_registros[i]);
        list_add(lista, registro_str);
    }

    return lista;
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

    act_cont->contexto = malloc(sizeof(t_contexto_ejecucion));
    if(!act_cont->contexto) {
        printf("Error al asignar el contexto");
        free(act_cont);
        return NULL;
    }

    memcpy(&(act_cont->contexto), stream + desplazamiento, sizeof(t_contexto_ejecucion));

    return act_cont;
}

int enviar_instruccion(int socket, t_instruccion* inst) {
    t_paquete* paquete = crear_paquete_con_codigo_de_operacion(INSTRUCCION);
    uint32_t tam_nom = strlen(inst->nombre);
    agregar_a_paquete(paquete, &tam_nom, sizeof(uint32_t));
    agregar_a_paquete(paquete, &inst->nombre, tam_nom);

    uint32_t tam_p1 = strlen(inst->parametro1);
    agregar_a_paquete(paquete, &tam_p1, sizeof(uint32_t));
    agregar_a_paquete(paquete, &inst->parametro1, tam_p1);

    uint32_t tam_p2 = strlen(inst->parametro2);
    agregar_a_paquete(paquete, &tam_p2, sizeof(uint32_t));
    agregar_a_paquete(paquete, &inst->parametro2, tam_p2);

    agregar_a_paquete(paquete, &inst->parametro3, sizeof(int));
   
    int resultado = enviar_paquete(paquete, socket);
    eliminar_paquete(paquete);

    return resultado;
}

void enviar_valor_leido_cpu(int socket, uint32_t dire_fisica, uint32_t valor) {
    t_paquete* paquete = crear_paquete_con_codigo_de_operacion(PEDIDO_READ_MEM);
    agregar_a_paquete(paquete, &dire_fisica, sizeof(uint32_t));
    agregar_a_paquete(paquete, &valor, sizeof(uint32_t));
   
    if(enviar_paquete(paquete, socket) == 0) {
        log_info(LOGGER_MEMORIA, "Valor enviado a Cpu");
    } else {
        log_error(LOGGER_MEMORIA, "Error al enviar el valor");
    }
    eliminar_paquete(paquete);
}

// Lista de instrucciones es de tipo t_proceso_instruccion
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


void procesar_solicitud_contexto(int socket_cliente, uint32_t pid, uint32_t tid) {
    for (int i = 0; i < list_size(lista_procesos); i++) {
        t_proceso_memoria* proceso = list_get(lista_procesos, i);
        
        if (proceso->pid == pid) {
            int resultado = enviar_contexto_cpu(proceso);
            if(resultado == 0) {
                log_info(LOGGER_MEMORIA, "## Contexto <Solicitado> - (PID:TID) - (<%d>:<%d>)", pid, tid);
                return;
            } else {
                break;
            }
        }
    }

    log_error(LOGGER_MEMORIA, "No se encontró contexto para el PID: %d, TID: %d", pid, tid);
    enviar_mensaje("ERROR", socket_memoria_cpu);
} 

int enviar_contexto_cpu(t_proceso_memoria* proceso) {
    t_paquete* paquete = crear_paquete_con_codigo_de_operacion(CONTEXTO);
    agregar_a_paquete(paquete, &proceso->pid, sizeof(uint32_t));
    agregar_a_paquete(paquete, &proceso->contexto->registros, sizeof(t_registros));
    agregar_a_paquete(paquete, &proceso->contexto->motivo_finalizacion, sizeof(finalizacion_proceso));
       
    int resultado = enviar_paquete(paquete, socket_memoria_cpu);
    eliminar_paquete(paquete);
    return resultado;
}

void procesar_actualizacion_contexto(int socket_cliente, uint32_t pid, uint32_t tid, t_contexto_ejecucion* nuevo_contexto) {
    for (int i = 0; i < list_size(lista_procesos); i++) {
        t_proceso_memoria* proceso = list_get(lista_procesos, i);
        
        if (proceso->pid == pid) {
            proceso->contexto = nuevo_contexto;
            log_info(LOGGER_MEMORIA, "## Contexto <Actualizado> - (PID:TID) - (<%d>:<%d)", pid, tid);
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
    agregar_a_paquete(paquete, &tamanio_nombre, sizeof(uint32_t));
    agregar_a_paquete(paquete, &nombre_archivo, tamanio_nombre); //
    agregar_a_paquete(paquete, &tamanio, sizeof(uint32_t));
    agregar_a_paquete(paquete, &contenido_proceso, tamanio);// 
   
    int resultado = enviar_paquete(paquete, socket_memoria_filesystem);
    eliminar_paquete(paquete);
    
    return resultado;
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

void escribir_memoria(uint32_t direccion_fisica, uint32_t valor) {
    // Verificar que la dirección física esté dentro de los límites de la memoria de usuario
    if (direccion_fisica + sizeof(uint32_t) > TAM_MEMORIA) {
        log_error(LOGGER_MEMORIA, "Error de segmentación: Dirección física %d fuera de los límites de memoria de usuario", direccion_fisica);
        return;
    }

    // Escribir los 4 bytes de `valor` a partir de `direccion_fisica`
    *(uint32_t*)(uintptr_t)direccion_fisica = valor;
    
    enviar_mensaje("OK", socket_memoria_cpu);  // Responder OK al cliente
}

// ========|
// READ_MEM|
// ========|

t_read_mem recibir_read_mem(int socket) {
    t_paquete* paquete = recibir_paquete(socket);
    t_read_mem read_mem = deserializar_read_mem(paquete->buffer);
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

    // Leer los primeros 4 bytes a partir de la dirección física
    uint32_t valor = *(uint32_t*)(uintptr_t)direccion_fisica;

    // Log de éxito
    log_info(LOGGER_MEMORIA, "Lectura exitosa: Dirección física %d, Valor %d", direccion_fisica, valor);

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
    agregar_a_paquete(paquete, &valor, sizeof(uint32_t));
    int resultado = enviar_paquete(paquete, socket);
    eliminar_paquete(paquete);
    return resultado;
}