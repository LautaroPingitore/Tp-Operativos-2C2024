#include "include/gestor.h"

void enviar_proceso_memoria(int socket_cliente, t_pcb* pcb, op_code codigo) {
    t_paquete* paquete = crear_paquete_con_codigo_operacion(codigo);    

    agregar_a_paquete(paquete, &pcb->PID, sizeof(uint32_t));
    agregar_a_paquete(paquete, &pcb->TAMANIO, sizeof(int));
    agregar_a_paquete(paquete, pcb->CONTEXTO, sizeof(t_contexto_ejecucion));
    serializar_paquete(paquete, paquete->buffer->size);

    if (enviar_paquete(paquete, socket_cliente) == -1) {
        log_error(LOGGER_KERNEL, "Error al enviar el proceso a memoria");
    } else {
        log_info(LOGGER_KERNEL,"Proceso enviado correctamente a memoria");
    }

    eliminar_paquete(paquete);
}

void enviar_proceso_cpu(int socket, t_pcb* pcb) {
    t_paquete* paquete = crear_paquete_con_codigo_operacion(SOLICITUD_PROCESO);
    agregar_a_paquete(paquete, &pcb->PID, sizeof(uint32_t));
    agregar_a_paquete(paquete, &pcb->CONTEXTO, sizeof(uint32_t));
    serializar_paquete(paquete, paquete->buffer->size);

    if(enviar_paquete(paquete, socket) == 0) {
        log_info(LOGGER_KERNEL, "Proceso enviado ok a cpu");
    } else {
        log_error(LOGGER_KERNEL, "No se a podido enviar el proceso requerido hacia el modulo de cpu :(");
    }
    eliminar_paquete(paquete);
}

int respuesta_memoria_creacion(int socket_cliente) {
    char buffer[50];
    int bytes_recibidos = recv(socket_cliente, buffer, sizeof(buffer)-1, 0);

    if (bytes_recibidos <= 0) {
        perror("Error al recibir la respuesta de memoria");
        return -1;
    }

    buffer[bytes_recibidos] = '\0';  // Asegurar que el mensaje esté terminado en NULL

    if(strcmp(buffer, "OK")) {
        return 1;
    } else {
        return -1;
    }

    free(buffer);
    return -1;
}

void envio_hilo_crear(int socket_cliente, t_tcb* tcb, op_code codigo) {
    t_paquete* paquete = crear_paquete_con_codigo_operacion(codigo);

    agregar_a_paquete(paquete, &tcb->TID, sizeof(uint32_t));
    agregar_a_paquete(paquete, &tcb->PID_PADRE, sizeof(uint32_t));
    agregar_a_paquete(paquete, &tcb->archivo, sizeof(strlen(tcb->archivo) + 1));
    serializar_paquete(paquete, paquete->buffer->size);

    if (enviar_paquete(paquete, socket_cliente) == -1) {
        log_error(LOGGER_KERNEL, "Error al enviar el proceso a memoria");
    } else {
        log_info(LOGGER_KERNEL,"Proceso enviado correctamente a memoria");
    }

    eliminar_paquete(paquete);

}

int enviar_hilo_a_cpu(t_tcb* hilo) {
    t_paquete* paquete = crear_paquete_con_codigo_operacion(HILO);
    agregar_a_paquete(paquete, &hilo->TID, sizeof(uint32_t));
    agregar_a_paquete(paquete, &hilo->PRIORIDAD, sizeof(hilo->PRIORIDAD));
    agregar_a_paquete(paquete, &hilo->PID_PADRE, sizeof(uint32_t));
    agregar_a_paquete(paquete, &hilo->ESTADO, sizeof(hilo->ESTADO));
    agregar_a_paquete(paquete, &hilo->PC, sizeof(uint32_t));
    serializar_paquete(paquete, paquete->buffer->size);

    int resultado = enviar_paquete(paquete, socket_kernel_cpu_dispatch);
    if(resultado == -1) {
        eliminar_paquete(paquete);
        return resultado;
    }

    log_info(LOGGER_KERNEL, "El TID %d se envio a CPU", hilo->TID);
    eliminar_paquete(paquete);

    return resultado;
}


void enviar_memory_dump(t_pcb* pcb, uint32_t tid) {
    t_paquete* paquete = crear_paquete_con_codigo_operacion(DUMP_MEMORY);
    agregar_a_paquete(paquete, &pcb->PID, sizeof(uint32_t));
    agregar_a_paquete(paquete, &tid, sizeof(uint32_t));
    serializar_paquete(paquete, paquete->buffer->size);

    if (enviar_paquete(paquete, socket_kernel_memoria) == -1) {
        log_error(LOGGER_KERNEL, "Error al enviar la solicitud de DUMP_MEMORY al módulo de memoria");
        // Mover el proceso a EXIT en caso de error
        process_exit(pcb);
        eliminar_paquete(paquete);
        return;
    }
    eliminar_paquete(paquete);
}

// REVISAR SI PUEDE HABER MAS DE UNA INTERRUPCION
// ACA SOLO ESTA PUESTA LA DE FIN DE QUANTUM
void enviar_interrupcion_cpu(op_code interrupcion, int quantum) {
    t_paquete* paquete = crear_paquete_con_codigo_operacion(interrupcion);
    bool hay_interrupcion = true;
    agregar_a_paquete(paquete, &quantum, sizeof(int));
    agregar_a_paquete(paquete, &hay_interrupcion, sizeof(bool));
    serializar_paquete(paquete, paquete->buffer->size);

    if(enviar_paquete(paquete, socket_kernel_cpu_interrupt) == -1) {
        log_error(LOGGER_KERNEL, "Error al enviar la interrupcion");
        // reiniciar_quantum()
        eliminar_paquete(paquete);
        return;
    }
    eliminar_paquete(paquete);
}

void* procesar_comunicaciones_cpu(void* void_args) {
    t_procesar_conexion_args *args = (t_procesar_conexion_args *)void_args;
    t_log *logger = args->log;
    int socket = args->fd;
    char *server_name = args->server_name;
    free(args);

    while(socket != -1) {
        t_paquete* paquete = recibir_paquete_entero(socket);
        void* stream = paquete->buffer->stream;
        int size = paquete->buffer->size;

        switch (paquete->codigo_operacion) {
        case HANDSHAKE_cpu:
            recibir_mensaje(socket, logger);
            log_info(logger, "Conexion establecida con CPU");
            break;
        
        case DEVOLVER_CONTROL_KERNEL:
            t_tcb* tcb = deserializar_paquete_tcb(stream, size);
            if(tcb->motivo_desalojo == INTERRUPCION_BLOQUEO) {
                list_remove(cola_exec, 0);

                pthread_mutex_lock(&mutex_cola_ready);
                list_add(cola_ready);
                pthread_mutex_unlock(&mutex_cola_ready);

                intentar_mover_a_execute();
            }
            break;

        default:
            manejar_syscall(paquete);
            break;
        }

        eliminar_paquete(paquete);
    }
    return NULL;
}