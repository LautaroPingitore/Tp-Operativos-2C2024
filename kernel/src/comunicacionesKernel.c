#include "include/gestor.h"

int respuesta_memoria = -1;

void enviar_proceso_memoria(int socket_cliente, t_pcb* pcb, op_code codigo) {
    t_paquete* paquete = crear_paquete_con_codigo_de_operacion(codigo);    

    paquete->buffer->size = sizeof(uint32_t) + sizeof(int) + sizeof(t_registros);
    paquete->buffer->stream = malloc(paquete->buffer->size);
    if (paquete->buffer->stream == NULL) {
        log_error(LOGGER_KERNEL, "Error al asignar memoria para el buffer de serialización");
        eliminar_paquete(paquete);
        return;
    }

    int desplazamiento = 0;

    // Serializar el PID
    memcpy(paquete->buffer->stream + desplazamiento, &(pcb->PID), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);

    // Serializar el tamaño
    memcpy(paquete->buffer->stream + desplazamiento, &(pcb->TAMANIO), sizeof(int));
    desplazamiento += sizeof(int);

    // Serializar el contexto de ejecución
    memcpy(paquete->buffer->stream + desplazamiento, pcb->CONTEXTO->registros, sizeof(t_registros));

    // Enviar el paquete
    if (enviar_paquete(paquete, socket_cliente) == -1) {
        log_error(LOGGER_KERNEL, "Error al enviar el proceso a memoria");
    } else {
        log_info(LOGGER_KERNEL, "Proceso enviado correctamente a memoria");
    }

    // Liberar recursos
    eliminar_paquete(paquete);
}

void enviar_proceso_cpu(int socket, t_pcb* pcb) {
    t_paquete* paquete = crear_paquete_con_codigo_de_operacion(SOLICITUD_PROCESO);

    paquete->buffer->size = sizeof(uint32_t) + sizeof(t_registros);
    paquete->buffer->stream = malloc(paquete->buffer->size);

    int desplazamiento = 0;
    memcpy(paquete->buffer->stream + desplazamiento, &(pcb->PID), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(paquete->buffer->stream + desplazamiento, pcb->CONTEXTO->registros, sizeof(t_registros));    

    if(enviar_paquete(paquete, socket) != 0) {
        log_error(LOGGER_KERNEL, "No se a podido enviar el proceso requerido hacia el modulo de cpu :(");
    }
    eliminar_paquete(paquete);
}

int respuesta_memoria_creacion(int socket_cliente) {
    // Recibimos el paquete completo desde memoria
    t_paquete* paquete = recibir_paquete(socket_cliente);

    if (paquete == NULL) {
        log_error(LOGGER_KERNEL, "Error al recibir respuesta de memoria");
        return -1;  // Error al recibir la respuesta
    }

    // Verificamos si la operación es MENSAJE (la respuesta que esperamos de memoria)
    if (paquete->codigo_operacion == MENSAJE) {
        char* respuesta = deserializar_mensaje(paquete->buffer);

        // Comparamos la respuesta con "OK"
        if (strcmp(respuesta, "OK") == 0) {
            free(paquete);  // Liberamos memoria del paquete
            return 1;  // Proceso creado correctamente
        } else {
            log_warning(LOGGER_KERNEL, "Error en la creación del proceso: %s", respuesta);
        }
    }

    free(paquete);  // Liberamos memoria del paquete
    return -1;  // Error en la creación del proceso
}

void envio_hilo_crear(int socket_cliente, t_tcb* tcb, op_code codigo) {
    t_paquete* paquete = crear_paquete_con_codigo_de_operacion(codigo);
    uint32_t tamanio_archivo = strlen(tcb->archivo) + 1;

    paquete->buffer->size = sizeof(uint32_t) * 3 + tamanio_archivo;
    paquete->buffer->stream = malloc(paquete->buffer->size);
    int desplazamiento = 0;

    memcpy(paquete->buffer->stream + desplazamiento, &(tcb->TID), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(paquete->buffer->stream + desplazamiento, &(tcb->PID_PADRE), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(paquete->buffer->stream + desplazamiento, &tamanio_archivo, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(paquete->buffer->stream + desplazamiento, tcb->archivo, tamanio_archivo);

    if (enviar_paquete(paquete, socket_cliente) == -1) {
        log_error(LOGGER_KERNEL, "Error al enviar el hilo a memoria");
    } else {
        log_info(LOGGER_KERNEL, "Hilo enviado correctamente a memoria");
    }

    eliminar_paquete(paquete);

}

int enviar_hilo_a_cpu(t_tcb* hilo) {
    t_paquete* paquete = crear_paquete_con_codigo_de_operacion(HILO);

    paquete->buffer->size = sizeof(uint32_t) * 3 + sizeof(int) + sizeof(motivo_desalojo);
    paquete->buffer->stream = malloc(paquete->buffer->size);
    int desplazamiento = 0;

    memcpy(paquete->buffer->stream + desplazamiento, &(hilo->TID), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);

    memcpy(paquete->buffer->stream + desplazamiento, &(hilo->PRIORIDAD), sizeof(int));
    desplazamiento += sizeof(int);

    memcpy(paquete->buffer->stream + desplazamiento, &(hilo->PID_PADRE), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);

    memcpy(paquete->buffer->stream + desplazamiento, &(hilo->motivo_desalojo), sizeof(motivo_desalojo));
    desplazamiento += sizeof(motivo_desalojo);

    memcpy(paquete->buffer->stream + desplazamiento, &(hilo->PC), sizeof(uint32_t));

    int resultado = enviar_paquete(paquete, socket_kernel_cpu_dispatch);
    if(resultado == -1) {
        eliminar_paquete(paquete);
        return resultado;
    }

    eliminar_paquete(paquete);

    return resultado;
}


void enviar_memory_dump(t_pcb* pcb, uint32_t tid) {
    t_paquete* paquete = crear_paquete_con_codigo_de_operacion(DUMP_MEMORY);

    paquete->buffer->size = sizeof(uint32_t) * 2;
    paquete->buffer->stream = malloc(paquete->buffer->size);
    int desplazamiento = 0;

    memcpy(paquete->buffer->stream + desplazamiento, &(pcb->PID), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);

    memcpy(paquete->buffer->stream + desplazamiento, &(tid), sizeof(uint32_t));

    if (enviar_paquete(paquete, socket_kernel_memoria) == -1) {
        log_error(LOGGER_KERNEL, "Error al enviar la solicitud de DUMP_MEMORY al módulo de memoria");
        // Mover el proceso a EXIT en caso de error
        process_exit(pcb);
        eliminar_paquete(paquete);
        return;
    } else {
        log_info(LOGGER_KERNEL, "Solicitud Dump Memory enviada correctamente");
    }
    eliminar_paquete(paquete);
}

// REVISAR SI PUEDE HABER MAS DE UNA INTERRUPCION
// ACA SOLO ESTA PUESTA LA DE FIN DE QUANTUM
void enviar_interrupcion_cpu(op_code interrupcion) {
    t_paquete *paquete = malloc(sizeof(t_paquete));
	
    paquete->codigo_operacion = interrupcion;
    paquete->buffer = malloc(sizeof(t_buffer));
    paquete->buffer->size = 0;
    paquete->buffer->stream = NULL;

    void *a_enviar = malloc(sizeof(op_code) + sizeof(uint32_t));
    int offset = 0;

    memcpy(a_enviar + offset, &(paquete->codigo_operacion), sizeof(op_code));
    offset += sizeof(op_code);
    memcpy(a_enviar + offset, &(paquete->buffer->size), sizeof(uint32_t));

    if (send(socket_kernel_cpu_interrupt, a_enviar, offset, 0) == -1) {
        perror("Error al enviar la interrupcion");
    }

    free(a_enviar);
    eliminar_paquete(paquete);
}

t_instruccion* recibir_instruccion(int socket) {
    t_paquete* paquete = recibir_paquete(socket);
    t_instruccion* inst = deserializar_instruccion(paquete->buffer);
    eliminar_paquete(paquete);
    return inst;
}