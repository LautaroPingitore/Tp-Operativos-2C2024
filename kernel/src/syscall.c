#include "include/gestor.h"

void syscall_process_create(char* path_proceso, int tamanio_proceso, int prioridad) {
    if (path_proceso == NULL || tamanio_proceso <= 0 || prioridad < 0) {
        log_error(LOGGER_KERNEL, "Syscall PROCESS_CREATE: Argumentos inválidos");
        return;
    }
    crear_proceso(path_proceso, tamanio_proceso, prioridad);
    log_info(LOGGER_KERNEL, "Syscall PROCESS_CREATE ejecutada para %s con tamanio %d y prioridad %d", path_proceso, tamanio_proceso, prioridad);
}

void syscall_process_exit(t_pcb* pcb) {
    if (pcb == NULL) {
        log_error(LOGGER_KERNEL, "Syscall PROCESS_EXIT: Argumento PCB nulo");
        return;
    }
    log_info(LOGGER_KERNEL, "Syscall PROCESS_EXIT ejecutada para proceso %d", pcb->PID);
    process_exit(pcb);
}


void syscall_thread_create(t_pcb* pcb, char* archivo_pseudocodigo, int prioridad){
    if (pcb == NULL || archivo_pseudocodigo == NULL || prioridad < 0) {
        log_error(LOGGER_KERNEL, "Syscall THREAD_CREATE: Argumentos inválidos");
        return;
    }
    log_info(LOGGER_KERNEL, "Syscall THREAD_CREATE ejecutada en proceso %d con prioridad %d", pcb->PID, prioridad);
    thread_create(pcb, archivo_pseudocodigo, prioridad);
}

void syscall_thread_join(t_pcb* pcb, uint32_t tid_actual, uint32_t tid_esperado) {
    if (pcb == NULL) {
        log_error(LOGGER_KERNEL, "Syscall THREAD_JOIN: PCB nulo");
        return;
    }
    log_info(LOGGER_KERNEL, "Syscall THREAD_JOIN ejecutada en proceso %d: hilo %d espera a hilo %d", pcb->PID, tid_actual, tid_esperado);
    thread_join(pcb, tid_actual, tid_esperado);
}

void syscall_thread_cancel(t_pcb* pcb, uint32_t tid) {
    if (pcb == NULL) {
        log_error(LOGGER_KERNEL, "Syscall THREAD_CANCEL: PCB nulo");
        return;
    }
    log_info(LOGGER_KERNEL, "Syscall THREAD_CANCEL ejecutada en proceso %d para hilo %d", pcb->PID, tid);
    thread_cancel(pcb, tid);
}

void syscall_thread_exit(t_pcb* pcb, uint32_t tid) {
    if (pcb == NULL) {
        log_error(LOGGER_KERNEL, "Syscall THREAD_EXIT: PCB nulo");
        return;
    }
    log_info(LOGGER_KERNEL, "Syscall THREAD_EXIT ejecutada por hilo %d en proceso %d", tid, pcb->PID);
    thread_exit(pcb, tid);
}

pthread_mutex_t* syscall_mutex_create() {
    pthread_mutex_t* mutex = malloc(sizeof(pthread_mutex_t));
    if (mutex == NULL) {
        log_error(LOGGER_KERNEL, "Syscall MUTEX_CREATE: No se pudo asignar memoria para el mutex");
        return NULL;
    }
    pthread_mutex_init(mutex, NULL);
    log_info(LOGGER_KERNEL, "Syscall MUTEX_CREATE ejecutada, nuevo mutex creado.");
    return mutex;
}

void syscall_mutex_lock(pthread_mutex_t* mutex) {
    if (mutex == NULL) {
        log_error(LOGGER_KERNEL, "Syscall MUTEX_LOCK: Mutex nulo");
        return;
    }
    pthread_mutex_lock(mutex);
    log_info(LOGGER_KERNEL, "Syscall MUTEX_LOCK ejecutada.");
}

void syscall_mutex_unlock(pthread_mutex_t* mutex) {
    if (mutex == NULL) {
        log_error(LOGGER_KERNEL, "Syscall MUTEX_UNLOCK: Mutex nulo");
        return;
    }
    pthread_mutex_unlock(mutex);
    log_info(LOGGER_KERNEL, "Syscall MUTEX_UNLOCK ejecutada.");
}

void syscall_dump_memory(t_pcb* pcb, uint32_t tid) {
    log_info(LOGGER_KERNEL, "Syscall DUMP_MEMORY ejecutada.");
    
    t_paquete* paquete = crear_paquete_con_codigo_operacion(DUMP_MEMORY);
    agregar_a_paquete(paquete, &pcb->PID, sizeof(pcb->PID));
    agregar_a_paquete(paquete, &tid, sizeof(tid));

    if (enviar_paquete(paquete, socket_kernel_memoria) == -1) {
        log_error(LOGGER_KERNEL, "Error al enviar la solicitud de DUMP_MEMORY al módulo de memoria");
        // Mover el proceso a EXIT en caso de error
        process_exit(pcb);
        eliminar_paquete(paquete);
        return;
    }

    eliminar_paquete(paquete);

    // Bloquear el hilo actual mientras espera la respuesta del módulo de memoria
    log_info(LOGGER_KERNEL, "Esperando confirmación del módulo de memoria...");
    char buffer[10];
    int bytes_recibidos = recv(socket_kernel_memoria, buffer, sizeof(buffer) - 1, 0);

    if (bytes_recibidos <= 0) {
        log_error(LOGGER_KERNEL, "Error al recibir respuesta del módulo de memoria");
        // Mover el proceso a EXIT en caso de error
        process_exit(pcb);
        return;
    }

    buffer[bytes_recibidos] = '\0';  // Asegurar que el mensaje recibido esté terminado en NULL

    if (strcmp(buffer, "OK") == 0) {
        log_info(LOGGER_KERNEL, "DUMP_MEMORY completado correctamente para proceso %d, hilo %d", pcb->PID, tid);

        // Mover el hilo a READY
        pthread_mutex_lock(&mutex_ready);
        t_tcb* hilo = buscar_hilo_por_tid(pcb, tid);
        if (hilo) {
            hilo->ESTADO = READY;
            list_add(cola_ready, hilo);
        }
        pthread_mutex_unlock(&mutex_ready);
    } else {
        log_error(LOGGER_KERNEL, "Error reportado por el módulo de memoria durante el DUMP_MEMORY");
        // Mover el proceso a EXIT en caso de error
        process_exit(pcb);
    }
}

void syscall_io(t_pcb* pcb, uint32_t tid, int milisegundos) {
    if (pcb == NULL || milisegundos <= 0) {
        log_error(LOGGER_KERNEL, "Syscall IO: Argumentos inválidos");
        return;
    }
    log_info(LOGGER_KERNEL, "Syscall IO ejecutada por hilo %d en proceso %d durante %d ms", tid, pcb->PID, milisegundos);
    io(pcb, tid, milisegundos);  // Invoca la funcion que gestiona la operacion IO
}

void manejar_syscall(op_code syscall, t_pcb* pcb, char* path_proceso, int tamanio, int prioridad,
                     uint32_t tid_actual, uint32_t tid_esperado, pthread_mutex_t* mutex, int milisegundos) {
    switch (syscall) {
        case PROCESS_CREATE:
            syscall_process_create(path_proceso, tamanio, prioridad);
            break;
        case PROCESS_EXIT:
            syscall_process_exit(pcb);
            break;
        case THREAD_CREATE:
            syscall_thread_create(pcb, path_proceso, prioridad);
            break;
        case THREAD_JOIN:
            syscall_thread_join(pcb, tid_actual, tid_esperado);
            break;
        case THREAD_CANCEL:
            syscall_thread_cancel(pcb, tid_actual);
            break;
        case THREAD_EXIT:
            syscall_thread_exit(pcb, tid_actual);
            break;
        case MUTEX_CREATE:
            syscall_mutex_create();
            break;
        case MUTEX_LOCK:
            syscall_mutex_lock(mutex);
            break;
        case MUTEX_UNLOCK:
            syscall_mutex_unlock(mutex);
            break;
        case DUMP_MEMORY:
            syscall_dump_memory(pcb, tid_actual);
            break;
        case IO:
            syscall_io(pcb, tid_actual, milisegundos);
            break;
        default:
            log_error(LOGGER_KERNEL, "Syscall desconocida: %d", syscall);
            break;
    }
}
