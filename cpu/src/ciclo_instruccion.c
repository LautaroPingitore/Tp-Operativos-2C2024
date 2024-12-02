#include "include/gestor.h"

// VARIABLES GLOBALES
t_tcb* hilo_actual = NULL;
int fd_cpu_memoria = -1;

void ejecutar_ciclo_instruccion(int socket_cliente) {
    while (true) {
        hilo_actual = recibir_hilo_kernel(socket_cliente);

        if (!hilo_actual) {
            log_error(LOGGER_CPU, "No hay Hilo actual. Terminando ciclo de instrucción.");
            break;
        }

        t_instruccion *instruccion = fetch(hilo_actual->TID, hilo_actual->PC);
        
        if (!instruccion) {
            log_error(LOGGER_CPU, "Error al obtener la instrucción. Terminando ciclo.");
            break;
        }

        // Aquí se llama a decode, pero por simplicidad se omite en este ejemplo
        execute(instruccion, socket_cliente);
        
        // Verifica si se necesita realizar un check_interrupt
        check_interrupt();

        liberar_instruccion(instruccion);
    }
}

// RECIBE LA PROXIMA EJECUCION A REALIZAR OBTENIDA DEL MODULO MEMORIA
t_instruccion *fetch(uint32_t tid, uint32_t pc) {
    pedir_instruccion_memoria(tid, pc, fd_cpu_memoria);
    
    t_paquete* paquete = recibir_paquete_entero(socket_cpu_dispatch_memoria);
    if (!paquete) {
        log_error(LOGGER_CPU, "Fallo al recibir paquete.");
        return NULL;
    }

    if (paquete->codigo_operacion != INSTRUCCION) {
        log_warning(LOGGER_CPU, "Operación desconocida al obtener la instrucción de memoria.");
        return NULL;
    }

    t_instruccion *instruccion = deserializar_instruccion(paquete->buffer->stream, paquete->buffer->size);
    if (!instruccion) {
        log_error(LOGGER_CPU, "Fallo al deserializar la instrucción.");
        return NULL;
    }

    log_info(LOGGER_CPU, "TID: %d - FETCH - Program Counter: %d", tid, pc);

    return instruccion;
}

// EJECUTA LA INSTRUCCION OBTENIDA, Y TAMBIEN HACE EL DECODE EN CASO DE NECESITARLO
void execute(t_instruccion *instruccion, int socket) {
    switch (instruccion->nombre) {
        case "SUM":
            loguear_y_sumar_pc(instruccion);
            sum_registros(instruccion->parametro1, instruccion->parametro2);
            break;
        case "READ_MEM":
            loguear_y_sumar_pc(instruccion);
            read_mem(instruccion->parametro1, instruccion->parametro2, socket);
            break;
        case "WRITE_MEM":
            loguear_y_sumar_pc(instruccion);
            write_mem(instruccion->parametro1, instruccion->parametro2, socket);
            break;
        case "JNZ":
            loguear_y_sumar_pc(instruccion);
            jnz_pc(instruccion->parametro1, instruccion->parametro2);
            break;
        case "SET":
            loguear_y_sumar_pc(instruccion);
            set_registro(instruccion->parametro1, instruccion->parametro2);
            break;
        case "SUB":
            loguear_y_sumar_pc(instruccion);
            sub_registros(instruccion->parametro1, instruccion->parametro2);
            break;
        case "LOG":
            loguear_y_sumar_pc(instruccion);
            break;

        // SYSCALLS QUE DEVUELVEN EL CONTROL A KERNEL
        case "DUMP_MEMORY":
            loguear_y_sumar_pc(instruccion);
            enviar_syscall_kernel(instruccion, DUMP_MEMORY);
            break;
        case "IO":
            loguear_y_sumar_pc(instruccion);
            enviar_syscall_kernel(instruccion, IO);
            break;
        case "PROCESS_CREATE":
            loguear_y_sumar_pc(instruccion);
            enviar_syscall_kernel(instruccion, PROCESS_CREATE);
            break;
        case "THREAD_CREATE":
            loguear_y_sumar_pc(instruccion);
            enviar_syscall_kernel(instruccion, THREAD_CREATE);
            break;
        case "THREAD_JOIN":
            loguear_y_sumar_pc(instruccion);
            enviar_syscall_kernel(instruccion, THREAD_JOIN);
            break;
        case "THREAD_CANCEL":
            loguear_y_sumar_pc(instruccion);
            enviar_syscall_kernel(instruccion, THREAD_CANCEL);
            break;
        case "MUTEX_CREATE":
            loguear_y_sumar_pc(instruccion);
            enviar_syscall_kernel(instruccion, MUTEX_CREATE);
            break;
        case "MUTEX_LOCK":
            loguear_y_sumar_pc(instruccion);
            enviar_syscall_kernel(instruccion, MUTEX_LOCK);
            break;
        case "MUTEX_UNLOCK":
            loguear_y_sumar_pc(instruccion);
            enviar_syscall_kernel(instruccion, MUTEX_UNLOCK);
            break;
        case "THREAD_EXIT":
            loguear_y_sumar_pc(instruccion);
            enviar_syscall_kernel(instruccion, THREAD_EXIT);
            break;
        case "PROCESS_EXIT":
            loguear_y_sumar_pc(instruccion);
            enviar_syscall_kernel(instruccion, PROCESS_EXIT);
            break;
        default:
            log_warning(LOGGER_CPU, "Instrucción desconocida.");
            break;
    }

}

// VERIFICA SI SE RECIBIO UNA INTERRUPCION POR PARTE DE KERNEL
// PODRIA RECIBIR UN PAQUETE CON EL NOMBRE DE LA INTERRUPCCION
void check_interrupt() {
    bool respuesta = recibir_interrupcion(socket_cpu_dispatch_kernel);
    if(respuesta) {
        log_info(LOGGER_CPU, "Interrupción recibida. Actualizando contexto y devolviendo control al Kernel.");
        actualizar_contexto_memoria(respuesta);
        devolver_control_al_kernel();
    }
}

// MUESTRA EN CONSOLA LA INSTRUCCION EJECUTADA Y LE SUMA 1 AL PC
void loguear_y_sumar_pc(t_instruccion *instruccion) {
    log_info(LOGGER_CPU, "TID: %d - Ejecutando: %s - Parametros: %s %s %d", hilo_actual->TID, instruccion->nombre, intruccion->parametro1, instruccion->parametro2, instruccion->parametro3);
    hilo_actual->PC++;
}

void liberar_instruccion(t_instruccion *instruccion) {
    free(instruccion->parametro1);
    free(instruccion->parametro2);
    free(instruccion->parametro3);
    free(instruccion);
}

void actualizar_contexto_memoria() {
    // Se puede suponer que la actualización del contexto implica actualizar los registros y el PC
    log_info(LOGGER_CPU, "Actualizando el contexto de la CPU en memoria...");
    if (!hilo_actual) {
        log_error(LOGGER_CPU, "No hay Hilo actual. No se puede actualizar contexto.");
        return;
    }

    t_tcb* pcb = obtener_pcb_padre_de_hilo(hilo_actual->PID_PADRE);

    // Enviar los registros y el program counter a memoria
    // A través de la memoria se actualizaría el PCB
    enviar_contexto_memoria(hilo_actual->PID_PADRE, hilo_actual->TID, pcb->CONTEXTO->registros, hilo_actual->PC, socket_cpu_dispatch_memoria);

    log_info(LOGGER_CPU, "Contexto de la CPU actualizado en memoria.");
}

void devolver_control_al_kernel() {
    log_info(LOGGER_CPU, "Devolviendo control al Kernel...");

    // Crear un paquete para notificar al Kernel
    t_paquete *paquete = crear_paquete_con_codigo_de_operacion(DEVOLVER_CONTROL_KERNEL);

    // Enviar el paquete indicando que el control se devuelve al Kernel
    enviar_paquete(paquete, socket_cpu_interrupt_kernel); 

    eliminar_paquete(paquete);

    log_info(LOGGER_CPU, "Control devuelto al Kernel.");
}