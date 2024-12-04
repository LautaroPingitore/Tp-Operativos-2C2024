#include "include/gestor.h"

bool hay_interrupcion = false;

// FUNCIONES DE CICLO_INSTRUCCION

t_tcb* recibir_hilo_kernel(int socket_cliente) {
    t_paquete* paquete = recibir_paquete(socket_cliente);
    if(paquete == NULL) {
        log_error(LOGGER_CPU, "Error al recibir el paquete del hilo.");
        return NULL;
    }

    if (paquete->codigo_operacion != HILO) {
        log_warning(LOGGER_CPU, "Código de operación inesperado: %d", paquete->codigo_operacion);
        eliminar_paquete(paquete);
        return NULL;
    }

    t_tcb* hilo = deserializar_paquete_tcb(paquete->buffer);

    log_info(LOGGER_CPU, "Hilo recibido: <%d>, PRIORIDAD=%d, PID_PADRE=%d, ESTADO=%d",
             hilo->TID, hilo->PRIORIDAD, hilo->PID_PADRE, hilo->ESTADO);

    eliminar_paquete(paquete);
    return hilo;
}

t_proceso_cpu* recibir_proceso(int socket) {
    t_paquete* paquete = recibir_paquete(socket);
    t_proceso_cpu* pcb = deserializar_proceso(paquete->buffer);
    eliminar_paquete(paquete);
    return pcb;
}

t_proceso_cpu* deserializar_proceso(t_buffer* buffer) {
    t_proceso_cpu* pcb = malloc(sizeof(t_pcb));
    void* stream = buffer->stream;
    int desplazamiento = 0;
    
    memcpy(&pcb->PID, stream + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);

    pcb->CONTEXTO = malloc(sizeof(t_contexto_ejecucion));
    if(!pcb->CONTEXTO) {
        printf("Error al crear el contexto");
        free(pcb);
        return NULL;
    }
    memcpy(&pcb->CONTEXTO, stream + desplazamiento, sizeof(t_contexto_ejecucion));

    return pcb;
}

void* recibir_interrupcion(void* void_args) {

    t_procesar_conexion_args *args = (t_procesar_conexion_args *)void_args;
    int socket = args->fd;

    t_paquete* paquete = recibir_paquete(socket);

    if(!paquete) {
        return NULL;
    }

    int quantum = 0;
    bool interrupcion = false;

    if(!deserializar_interrupcion(paquete->buffer->stream, paquete->buffer->size, quantum, interrupcion)) {
        log_error(LOGGER_CPU, "Error al deserializar los datos de interrupcion");
        eliminar_paquete(paquete);
        return NULL;
    }

    if(interrupcion) {
        log_info(LOGGER_CPU, "Interrupcion recibida, Quantum = %d", quantum);
        hay_interrupcion = true;
    }
    eliminar_paquete(paquete);
    return NULL;;
}

bool deserializar_interrupcion(void* stream, int size, int quantum, bool interrupcion) {

    if (size < sizeof(int) + sizeof(bool)) {
        return false;
    }

    memcpy(&quantum, stream, sizeof(int));
    memcpy(&interrupcion, stream + sizeof(int), sizeof(bool));

    return true;
}

void pedir_instruccion_memoria(uint32_t tid, uint32_t pc, int socket) {
    t_paquete *paquete = crear_paquete_con_codigo_de_operacion(PEDIDO_INSTRUCCION);
    agregar_a_paquete(paquete, &tid, sizeof(uint32_t));
    agregar_a_paquete(paquete, &pc, sizeof(uint32_t));
    serializar_paquete(paquete, paquete->buffer->size);

    enviar_paquete(paquete, socket);
    eliminar_paquete(paquete);
}

void enviar_contexto_memoria(uint32_t pid, uint32_t tid, t_registros* registros, uint32_t program_counter, int socket_memoria) {
    t_paquete *paquete = crear_paquete_con_codigo_de_operacion(ACTUALIZAR_CONTEXTO);

    agregar_a_paquete(paquete, &pid, sizeof(uint32_t));
    agregar_a_paquete(paquete, &tid, sizeof(uint32_t));
    agregar_a_paquete(paquete, &registros, sizeof(t_registros));
    agregar_a_paquete(paquete, &program_counter, sizeof(uint32_t));
    serializar_paquete(paquete, paquete->buffer->size);

    enviar_paquete(paquete, socket_memoria);
    eliminar_paquete(paquete);
}

void enviar_syscall_kernel(t_instruccion* instruccion, op_code syscall) {
    t_paquete* paquete = crear_paquete_con_codigo_de_operacion(syscall);
    agregar_a_paquete(paquete, &instruccion->nombre, sizeof(instruccion->nombre));
    agregar_a_paquete(paquete, &instruccion->parametro1, sizeof(instruccion->parametro1));
    agregar_a_paquete(paquete, &instruccion->parametro2, sizeof(instruccion->parametro2));
    agregar_a_paquete(paquete, &instruccion->parametro3, sizeof(instruccion->parametro3));

    if(enviar_paquete(paquete, socket_cpu_interrupt_kernel) == 0) {
        log_info(LOGGER_CPU, "Syscall notificada a KERNEL");
    } else {
        eliminar_paquete(paquete);
        log_error(LOGGER_CPU, "Error al enviar la notificacion");
    }
    eliminar_paquete(paquete);
}

// FUNCIONES INSTRUCCIONES

void enviar_interrupcion_segfault(uint32_t pid, int socket) {
    // Crear el paquete de interrupcion
    t_paquete* paquete = crear_paquete_con_codigo_de_operacion(SEGF_FAULT);
    
    // Agregar el PID al paquete
    agregar_a_paquete(paquete, &pid, sizeof(uint32_t));
    
    serializar_paquete(paquete, paquete->buffer->size);

    // Enviar el paquete al socket de la memoria o kernel
    if (enviar_paquete(paquete, socket) != 0) {
        log_error(LOGGER_CPU, "Error al enviar interrupción de Segmentation Fault para PID: %d", pid);
    } else {
        log_info(LOGGER_CPU, "Interrupción de Segmentation Fault enviada correctamente (PID: %d)", pid);
    }
    
    // Liberar el paquete
    eliminar_paquete(paquete);
}

void enviar_valor_a_memoria(int socket, uint32_t dire_fisica, uint32_t* valor) {
    t_paquete *paquete = crear_paquete_con_codigo_de_operacion(ESCRIBIR_VALOR_MEMORIA);

    // Agregar la dirección física y el valor del registro de datos al paquete
    agregar_a_paquete(paquete, &dire_fisica, sizeof(uint32_t));
    agregar_a_paquete(paquete, &valor, sizeof(uint32_t));

    serializar_paquete(paquete, paquete->buffer->size);

    if (enviar_paquete(paquete, socket) < 0) {
        log_error(LOGGER_CPU, "Error al enviar valor a memoria: Dirección Física %d - Valor %d", dire_fisica, *valor);
    } else {
        log_info(LOGGER_CPU, "Valor enviado a Memoria: Dirección Física %d - Valor %d", dire_fisica, *valor);
    }
    
    // Eliminar el paquete para liberar memoria
    eliminar_paquete(paquete);
}

void enviar_solicitud_valor_memoria(int socket, uint32_t direccion_fisica) {
    t_paquete *paquete = crear_paquete_con_codigo_de_operacion(PEDIR_VALOR_MEMORIA);
    agregar_a_paquete(paquete, &direccion_fisica, sizeof(uint32_t));

    if (enviar_paquete(paquete, socket) < 0) {
        log_error(LOGGER_CPU, "Error al enviar solicitud de valor desde memoria.");
        eliminar_paquete(paquete);
        return;
    }

    eliminar_paquete(paquete);
}

uint32_t recibir_valor_de_memoria(int socket) {
    t_paquete* paquete = recibir_paquete(socket);
    uint32_t valor;
    memcpy(&valor, paquete->buffer->stream, sizeof(uint32_t));
    eliminar_paquete(paquete);
    return valor;
}

// FUNCIONES MMU

void enviar_solicitud_memoria(int socket, uint32_t pid, op_code codigo, const char* descripcion) {
    t_paquete* paquete = crear_paquete_con_codigo_de_operacion(codigo);
    agregar_a_paquete(paquete, &pid, sizeof(uint32_t));
    serializar_paquete(paquete, paquete->buffer->size);

    if (enviar_paquete(paquete, socket) < 0) {
        log_error(LOGGER_CPU, "Error al enviar solicitud de %s para PID: %d", descripcion, pid);
    } else {
        log_info(LOGGER_CPU, "Solicitud de %s enviada para PID: %d", descripcion, pid);
    }

    eliminar_paquete(paquete);
}

uint32_t recibir_entero(int socket, const char* mensaje_error) {
    uint32_t valor;
    ssize_t bytes_recibidos = recv(socket, &valor, sizeof(uint32_t), MSG_WAITALL);

    if (bytes_recibidos <= 0) {
        log_error(LOGGER_CPU, "Error al recibir datos: %s", mensaje_error);
        return (uint32_t)-1;
    }

    return valor;
}