#include "include/gestor.h"

void enviar_proceso_memoria(int socket_cliente, t_pcb* pcb, op_code codigo) {
    t_paquete* paquete = crear_paquete_con_codigo_operacion(codigo);    

    agregar_a_paquete(paquete, &pcb->PID, sizeof(uint32_t));
    agregar_a_paquete(paquete, &pcb->TAMANIO, sizeof(int));
    agregar_a_paquete(paquete, pcb->CONTEXTO, sizeof(t_contexto_ejecucion));

    if (enviar_paquete(paquete, socket_cliente) == -1) {
        log_error(LOGGER_KERNEL, "Error al enviar el proceso a memoria");
    } else {
        log_info(LOGGER_KERNEL,"Proceso enviado correctamente a memoria");
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
    agregar_a_paquete(paquete, &tcb->archivo, sizeof(char*));

    if (enviar_paquete(paquete, socket_cliente) == -1) {
        log_error(LOGGER_KERNEL, "Error al enviar el proceso a memoria");
    } else {
        log_info(LOGGER_KERNEL,"Proceso enviado correctamente a memoria");
    }

    eliminar_paquete(paquete);

}

// MEMORIA
