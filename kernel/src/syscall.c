#include "include/gestor.h"

void syscall_process_create(char* path_proceso, int tamanio_proceso, int prioridad) {
    if (path_proceso == NULL || tamanio_proceso <= 0 || prioridad < 0) {
        log_error(LOGGER_KERNEL, "Syscall PROCESS_CREATE: Argumentos inválidos");
        return;
    }
    crear_proceso(path_proceso, tamanio_proceso, prioridad);
    log_info(LOGGER_KERNEL, "Syscall PROCESS_CREATE ejecutada para %s con tamanio %d y prioridad %d", path_proceso, tamanio_proceso, prioridad);

    pthread_mutex_lock(&mutex_cola_ready);
    list_add(cola_ready, hilo_actual);
    pthread_mutex_unlock(&mutex_cola_ready);
}

void syscall_process_exit(uint32_t pid) {
    if (pid == NULL) {
        log_error(LOGGER_KERNEL, "Syscall PROCESS_EXIT: Argumento PCB nulo");
        return;
    }
    t_pcb* pcb = obtener_pcb_padre_de_hilo(pid);
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

    pthread_mutex_lock(&mutex_cola_ready);
    list_add(cola_ready, hilo_actual);
    pthread_mutex_unlock(&mutex_cola_ready);
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

void syscall_mutex_create(uint32_t pid, char* nombre) {
    t_pcb* pcb = obtener_pcb_padre_de_hilo(pid);
    t_recurso* recurso = buscar_recurso_proceso(pid, nombre);

    if(recuros == NULL) {
        pthread_mutex_t* mutex = malloc(sizeof(pthread_mutex_t));
        t_recurso* recurso_nuevo = malloc(sizeof(t_recurso));
        recurso->nombre_recurso = nombre;
        recurso->mutex = mutex;
        pthread_mutex_init(mutex, NULL);

        list_add(pcb->MUTEXS, recurso_nuevo);

        log_info(LOGGER_KERNEL, "Syscall MUTEX_CREATE ejecutada, nuevo mutex creado.");
    } else {
        log_ifo(LOGGER_KERNEL, "EL mutex %s ya se encuentra creado en el proceso %d", nombre, pid);
    }

    pthread_mutex_lock(&mutex_cola_ready);
    list_add(cola_ready, hilo_actual);
    pthread_mutex_unlock(&mutex_cola_ready);
}

void syscall_mutex_lock(uint32_t pid, char* nombre) {
    t_recurso* recurso = buscar_recurso_proceso(pid, nombre);

    int resultado = pthread_mutex_trylock(&recurso->mutex);
    t_tcb* hilo_bloqueado = hilo_actual;

    if(resultado !=0) {
        // EL MUTEX YA ESTA TOMADO
        hilo_bloqueado->ESTADO = BLOCK_MUTEX;
        // AGREGO ESTE PORQUE TAL VEZ, CUANDO SE DESBLOQUEE EL HILO
        // ESTE SEA DISTINTO AL ACTUAL
        list_add(recurso->hilos_bloqueados, hilo_bloqueado);

        pthread_mutex_lock(&mutex_cola_blocked);
        list_add(cola_blocked, hilo_actual);
        pthread_mutex_unlock(&mutex_cola_blocked);

        // ESPERA A QUE SE LIBERE Y EL PRIMER RECURSO QUE SE
        // BLOQUEO POR EL MUTEX LO TOMA
        pthread_cond_wait(&recurso->cond_mutex, &recurso->mutex);

        pthread_mutex_lock(&mutex_cola_blocked);
        eliminar_tcb_lista(recurso->hilos_bloqueados, hilo_bloqueado);
        pthread_mutex_unlock(&mutex_cola_blocked);

        log_info(LOGGER_KERNEL, "(<%d>:<%d>) Desbloqueado del Mutex: %s", hilo_bloqueado->PID_PADRE, hilo_bloqueado->TID, recurso->nombre_recurso);
    } else {
        log_info(LOGGER_KERNEL, "Syscall MUTEX_LOCK ejecutada.");
    }

    pthread_mutex_lock(&mutex_cola_ready);
    list_add(cola_ready, hilo_bloqueado);
    pthread_mutex_unlock(&mutex_cola_ready);

}

void syscall_mutex_unlock(uint32_t pid, char* nombre) {
    t_recurso* recurso = buscar_recurso_proceso(pid, nombre);

    if(recurso->hilos_bloqueados > 0) {
        pthread_cond_signal(&recurso->cond_mutex);
    }
    pthread_mutex_unlock(&recurso->mutex);

    log_info(LOGGER_KERNEL, "Syscall MUTEX_UNLOCK ejecutada.");

    pthread_mutex_lock(&mutex_cola_ready);
    list_add(cola_ready, hilo_actual);
    pthread_mutex_unlock(&mutex_cola_ready);
}

void syscall_dump_memory(t_pcb* pcb, uint32_t tid) {
    log_info(LOGGER_KERNEL, "Syscall DUMP_MEMORY ejecutandose...");

    enviar_memory_dump(pcb, tid);

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
        pthread_mutex_lock(&mutex_cola_ready);
        t_tcb* hilo = buscar_hilo_por_tid(pcb, tid);
        if (hilo) {
            hilo->ESTADO = READY;
            list_add(cola_ready, hilo);
        }
        pthread_mutex_unlock(&mutex_cola_ready);
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

void manejar_syscall(t_paquete* paquete) {
    t_instruccion* inst = deserializar_instruccion(paquete->buffer->stream, paquete.buffer.size);
    switch (paquete->codigo_operacion) {
        case PROCESS_CREATE:
            int tamanio = atoi(inst->parametro2);
            log_syscall("PROCESS_CREATE", hilo_actual);
            syscall_process_create(inst->parametro1, tamanio, inst->parametro3);
            intentar_mover_a_execute();
            break;
        case PROCESS_EXIT:
            log_syscall("PROCESS_EXIT", hilo_actual);
            syscall_process_exit(hilo_actual->PID_PADRE);
            intentar_mover_a_execute();
            break;
        case THREAD_CREATE:
            int prioridad = atoi(inst->parametro2);
            log_syscall("THREAD_CREATE", hilo_actual);
            syscall_thread_create(hilo_actual->PID_PADRE, inst->parametro1, prioridad);
            intentar_mover_a_execute();
            break;
        case THREAD_JOIN:
            uint32_t tid_esperado = atoi(inst->parametro1);
            log_syscall("THREAD_JOIN", hilo_actual);
            syscall_thread_join(hilo_actual->PID_PADRE, hilo_actual->TID, tid_esperado);
            break;
        case THREAD_CANCEL:
            uint32_t tid = atoi(inst->parametro1);
            log_syscall("THREAD_CANCEL", hilo_actual);
            syscall_thread_cancel(hilo_actual->PID_PADRE, tid);
            intentar_mover_a_execute();
            break;
        case THREAD_EXIT:
            log_syscall("THREAD_CANCEL", hilo_actual);
            syscall_thread_exit(hilo_actual->PID_PADRE, hilo_actual->TID);
            intentar_mover_a_execute();
            break;
        case MUTEX_CREATE:
            log_syscall("MUTEX_CREATE", hilo_actual);
            syscall_mutex_create(hilo_actual->PID_PADRE, inst->parametro1);
            intentar_mover_a_execute();
            break;
        case MUTEX_LOCK:
            log_syscall("MUTEX_LOCK", hilo_actual);
            syscall_mutex_lock(inst->parametro1);
            intentar_mover_a_execute();
            break;
        case MUTEX_UNLOCK:
            log_syscall("MUTEX_UNLOCK", hilo_actual);
            syscall_mutex_unlock(inst->parametro1);
            intentar_mover_a_execute();
            break;
        case DUMP_MEMORY:
            log_syscall("DUMP_MEMORY", hilo_actual);
            syscall_dump_memory(hilo_actual->PID_PADRE, hilo_actual->TID);
            intentar_mover_a_execute();
            break;
        case IO:
            int milisegundos = atoi(inst->parametro1);
            log_syscall("IO", hilo_actual);
            syscall_io(hilo_actual->PID_PADRE, hilo_actual->TID, milisegundos);
            break;
        default:
            log_error(LOGGER_KERNEL, "Syscall desconocida");
            break;
    }

    free(paquete);
    free(inst);

    // UNA VEZ TERMINADA LA SYSCALL TRATA DE EJECUTAR EL PROXIMO HILO
    if(strcmp(ALGORITMO_PLANIFICACION, "COLAS_MULTINIVEL")) {
        reiniciar_quantum();
    }
}

void log_syscall(char* syscall, t_tcb* tcb) {
    log_info(LOGGER_KERNEL, "## (<%d>:<%d>) - Solicitó syscall: <%s>", tcb->PID_PADRE, tcb->TID, syscall);
}