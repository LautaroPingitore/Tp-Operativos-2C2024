#include "include/gestor.h"

// RECIBE EL HILO A EJECUTAR POR PARTE DE KERNEL
t_tcb* recibir_hilo_kernel(int socket_cliente) {
    t_paquete* paquete = recibir_paquete_entero(socket_cliente);
    if(paquete == NULL) {
        log_error(logger, "Error al recibir el paquete del hilo.");
        return;
    }

    if (paquete->codigo_operacion != HILO) {
        log_warning(logger, "Código de operación inesperado: %d", paquete->codigo_operacion);
        eliminar_paquete(paquete);
        return;
    }

    t_tcb* hilo = deserializar_paquete_tcb(paquete->buffer->stream, paquete->buffer->size);

    log_info(logger, "Hilo recibido: <%d>, PRIORIDAD=%d, PID_PADRE=%d, ESTADO=%d",
             hilo->TID, hilo->PRIORIDAD, hilo->PID_PADRE, hilo->ESTADO);

    liminar_paquete(paquete);
    return hilo;
}

bool recibir_interrupcion(int socket) {
    t_paquete* paquete = recibir_paquete_entero(socket);

    if(!paquete) {
        return false;
    }

    int quantum;
    bool interrupcion;

    if(!deserializar_instruccion(paquete->buffer->stream, paquete->buffer->size, quantum, interrupcion)) {
        log_error(LOGGER_CPU, "Error al deserializar los datos de interrupcion");
        eliminar_paquete(paquete);
        return false;
    }

    if(interrupcion) {
        log_info(LOGGER_CPU, "Interrupcion recibida, Quantum = %d", quantum);
    }
    eliminar_paquete(paquete);
    return interrupcion;
}

bool deserializar_interrupcion(void* stream, int size, int quantum, bool interrupcion, ) {

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
    serializar_paquete(paquete);

    enviar_paquete(paquete, socket);
    eliminar_paquete(paquete);
}

void enviar_contexto_memoria(uint32_t pid, uint32_t tid, t_registros* registros, uint32_t program_counter, int socket_memoria) {
    t_paquete *paquete = crear_paquete_con_codigo_de_operacion(ACTUALIZAR_CONTEXTO);

    agregar_a_paquete(paquete, &pid, sizeof(uint32_t));
    agregar_a_paquete(paquete, &tid, sizeof(uint32_t));
    agregar_a_paquete(paquete, &registros, sizeof(t_registros));
    agregar_a_paquete(paquete, &program_counter, sizeof(uint32_t));
    serializar_paquete(paquete);

    enviar_paquete(paquete, socket_memoria);
    eliminar_paquete(paquete);
}

void enviar_syscall_kernel(t_instruccion* instruccion, op_code syscall) {
    t_paquete* paquete = crear_paquete_con_codigo_operacion(syscall);
    agregar_a_paquete(paquete, &instruccion->nombre, sizeof(char*));
    agregar_a_paquete(paquete, &instruccion->parametro1, sizeof(char*));
    agregar_a_paquete(paquete, &instruccion->parametro2, sizeof(char*));
    agregar_a_paquete(paquete, &instruccion->parametro3, sizeof(int));

    if(enviar_paquete(paquete, socket_cliente) == 0) {
        log_info(LOGGER_CPU, "Syscall notificada a KERNEL");
    } else {
        eliminar_paquete(paquete);
        log_error(LOGGER_CPU, "Error al enviar la notificacion");
    }
    eliminar_paquete(paquete);
}