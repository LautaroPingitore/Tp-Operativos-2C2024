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

    pcb->REGISTROS = malloc(sizeof(t_registros));
    if(!pcb->REGISTROS) {
        printf("Error al crear los registros");
        free(pcb);
        return NULL;
    }
    memcpy((pcb->REGISTROS), stream + desplazamiento, sizeof(t_registros));

    return pcb;
}

t_instruccion* recibir_instruccion(int socket) {
    t_paquete* paquete = recibir_paquete(socket);
    t_instruccion* inst = deserializar_instruccion(paquete->buffer);
    eliminar_paquete(paquete);
    return inst;
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

void pedir_instruccion_memoria(uint32_t pid, uint32_t tid, uint32_t pc, int socket) {
    t_paquete *paquete = crear_paquete_con_codigo_de_operacion(PEDIDO_INSTRUCCION);
    
    paquete->buffer->size = sizeof(uint32_t) * 3;
    paquete->buffer->stream = malloc(paquete->buffer->size);

    int desplazamiento = 0;

    memcpy(paquete->buffer->stream + desplazamiento, &(pid), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(paquete->buffer->stream + desplazamiento, &(tid), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(paquete->buffer->stream + desplazamiento, &(pc), sizeof(uint32_t));

    enviar_paquete(paquete, socket);
    eliminar_paquete(paquete);
}

void enviar_contexto_memoria(uint32_t pid, uint32_t tid, t_registros* registros, int socket_memoria) {
    t_paquete *paquete = crear_paquete_con_codigo_de_operacion(ACTUALIZAR_CONTEXTO);

    paquete->buffer->size = sizeof(uint32_t) * 2 + sizeof(t_registros);
    paquete->buffer->stream = malloc(paquete->buffer->size);

    int desplazamiento = 0;

    memcpy(paquete->buffer->stream + desplazamiento, &(pid), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(paquete->buffer->stream + desplazamiento, &(tid), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    
    memcpy(paquete->buffer->stream + desplazamiento, registros, sizeof(t_registros));

    // memcpy(paquete->buffer->stream + desplazamiento, &(registros->AX), sizeof(uint32_t));
    // desplazamiento += sizeof(uint32_t);

    // memcpy(paquete->buffer->stream + desplazamiento, &(registros->BX), sizeof(uint32_t));
    // desplazamiento += sizeof(uint32_t);

    // memcpy(paquete->buffer->stream + desplazamiento, &(registros->CX), sizeof(uint32_t));
    // desplazamiento += sizeof(uint32_t);

    // memcpy(paquete->buffer->stream + desplazamiento, &(registros->DX), sizeof(uint32_t));
    // desplazamiento += sizeof(uint32_t);

    // memcpy(paquete->buffer->stream + desplazamiento, &(registros->EX), sizeof(uint32_t));
    // desplazamiento += sizeof(uint32_t);

    // memcpy(paquete->buffer->stream + desplazamiento, &(registros->FX), sizeof(uint32_t));
    // desplazamiento += sizeof(uint32_t);
    
    // memcpy(paquete->buffer->stream + desplazamiento, &(registros->GX), sizeof(uint32_t));
    // desplazamiento += sizeof(uint32_t);

    // memcpy(paquete->buffer->stream + desplazamiento, &(registros->HX), sizeof(uint32_t));
 
    enviar_paquete(paquete, socket_memoria);
    eliminar_paquete(paquete);
}

void enviar_syscall_kernel(t_instruccion* instruccion, op_code syscall) {
    t_paquete* paquete = crear_paquete_con_codigo_de_operacion(syscall);
 
    uint32_t tamanio_nombre = strlen(instruccion->nombre) + 1;
    uint32_t tamanio_par1 = strlen(instruccion->parametro1) + 1;
    uint32_t tamanio_par2 = strlen(instruccion->parametro2) + 1;

    paquete->buffer->size = tamanio_nombre + tamanio_par1 + tamanio_par2 + sizeof(int) + sizeof(uint32_t) * 3;

    paquete->buffer->stream=malloc(paquete->buffer->size);
    int desplazamiento=0;

    memcpy(paquete->buffer->stream + desplazamiento, instruccion->nombre, tamanio_nombre);
    desplazamiento += tamanio_nombre;
    memcpy(paquete->buffer->stream + desplazamiento, instruccion->parametro1, tamanio_par1);
    desplazamiento += tamanio_par1;
    memcpy(paquete->buffer->stream + desplazamiento, instruccion->parametro2, tamanio_par2);
    desplazamiento += tamanio_par2;
    memcpy(paquete->buffer->stream + desplazamiento, &(instruccion->parametro3), sizeof(int));
    

    if(enviar_paquete(paquete, socket_cpu_interrupt_kernel) == 0) {
        log_info(LOGGER_CPU, "Syscall notificada a KERNEL");
    } else {
        eliminar_paquete(paquete);
        log_error(LOGGER_CPU, "Error al enviar la notificacion");
    }
    eliminar_paquete(paquete);
}

void devolver_control_al_kernel() {
    log_info(LOGGER_CPU, "Devolviendo control al Kernel...");

    // Crear un paquete para notificar al Kernel
    t_paquete *paquete = crear_paquete_con_codigo_de_operacion(DEVOLVER_CONTROL_KERNEL);
    hilo_actual->motivo_desalojo = INTERRUPCION_BLOQUEO;

    paquete->buffer->size = sizeof(uint32_t) * 3 + sizeof(int) + sizeof(t_estado);
    paquete->buffer->stream = malloc(paquete->buffer->size);
    int desplazamiento = 0;

    memcpy(paquete->buffer->stream + desplazamiento, &(hilo_actual->TID), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);

    memcpy(paquete->buffer->stream + desplazamiento, &(hilo_actual->PRIORIDAD), sizeof(int));
    desplazamiento += sizeof(int);

    memcpy(paquete->buffer->stream + desplazamiento, &(hilo_actual->PID_PADRE), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);

    memcpy(paquete->buffer->stream + desplazamiento, &(hilo_actual->ESTADO), sizeof(t_estado));
    desplazamiento += sizeof(t_estado);

    memcpy(paquete->buffer->stream + desplazamiento, &(hilo_actual->PC), sizeof(uint32_t));
    
    enviar_paquete(paquete, socket_cpu_interrupt_kernel); 

    eliminar_paquete(paquete);

    log_info(LOGGER_CPU, "Control devuelto al Kernel.");
}

// FUNCIONES INSTRUCCIONES

void enviar_interrupcion_segfault(uint32_t pid, int socket) {
    // Crear el paquete de interrupcion
    t_paquete* paquete = crear_paquete_con_codigo_de_operacion(SEGF_FAULT);
    
    // Agregar el PID al paquete

    paquete->buffer->size = sizeof(uint32_t);
    paquete->buffer->stream = malloc(paquete->buffer->size);

    int desplazamiento = 0;

    memcpy(paquete->buffer->stream + desplazamiento, &(pid), sizeof(uint32_t));
    

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
    
    paquete->buffer->size = sizeof(uint32_t) * 4;
    paquete->buffer->stream = malloc(paquete->buffer->size);

    int desplazamiento = 0;

    memcpy(paquete->buffer->stream + desplazamiento, &(hilo_actual->PID_PADRE), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(paquete->buffer->stream + desplazamiento, &(hilo_actual->TID), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(paquete->buffer->stream + desplazamiento, &(dire_fisica), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(paquete->buffer->stream + desplazamiento, &(valor), sizeof(uint32_t));


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

    paquete->buffer->size = sizeof(uint32_t) * 3;
    paquete->buffer->stream = malloc(paquete->buffer->size);

    int desplazamiento = 0;

    memcpy(paquete->buffer->stream + desplazamiento, &(hilo_actual->PID_PADRE), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(paquete->buffer->stream + desplazamiento, &(hilo_actual->TID), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(paquete->buffer->stream + desplazamiento, &(direccion_fisica), sizeof(uint32_t));

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

    paquete->buffer->size = sizeof(uint32_t);
    paquete->buffer->stream = malloc(paquete->buffer->size);

    int desplazamiento = 0;

    memcpy(paquete->buffer->stream + desplazamiento, &(pid), sizeof(uint32_t));

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