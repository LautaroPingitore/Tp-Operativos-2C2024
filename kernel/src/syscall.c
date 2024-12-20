#include "include/gestor.h"

void syscall_process_create(t_tcb* hilo_actual, char* path_proceso, int tamanio_proceso, int prioridad) {
    if (!path_proceso || tamanio_proceso <= 0 || prioridad < 0) {
        log_error(LOGGER_KERNEL, "Syscall PROCESS_CREATE: Argumentos inválidos");
        return;
    }
    crear_proceso(path_proceso, tamanio_proceso, prioridad);
    log_info(LOGGER_KERNEL, "Syscall PROCESS_CREATE ejecutada para %s con tamanio %d y prioridad %d", path_proceso, tamanio_proceso, prioridad);

    mover_hilo_a_ready(hilo_actual);

}

void syscall_process_exit(uint32_t pid) {
    if (pid < 0) {
        log_error(LOGGER_KERNEL, "Syscall PROCESS_EXIT: Argumento PCB nulo");
        return;
    }
    t_pcb* pcb = obtener_pcb_padre_de_hilo(pid);
    log_info(LOGGER_KERNEL, "Syscall PROCESS_EXIT ejecutada para proceso %d", pcb->PID);
    process_exit(pcb);
}


void syscall_thread_create(t_tcb* hilo_actual, uint32_t pid, char* archivo_pseudocodigo, int prioridad){
    if (pid < 0 || !archivo_pseudocodigo || prioridad < 0) {
        log_error(LOGGER_KERNEL, "Syscall THREAD_CREATE: Argumentos inválidos");
        return;
    }
    
    t_pcb* pcb = obtener_pcb_padre_de_hilo(pid);

    log_info(LOGGER_KERNEL, "Syscall THREAD_CREATE ejecutada en proceso %d con prioridad %d", pcb->PID, prioridad);
    thread_create(pcb, archivo_pseudocodigo, prioridad);
    mover_hilo_a_ready(hilo_actual);
}

void syscall_thread_join(uint32_t pid, uint32_t tid_actual, uint32_t tid_esperado) {
    if (pid < 0) {
        log_error(LOGGER_KERNEL, "Syscall THREAD_JOIN: PCB nulo");
        return;
    }

    t_pcb* pcb = obtener_pcb_padre_de_hilo(pid);

    log_info(LOGGER_KERNEL, "## (<%d>:<%d>) - Bloqueado por <THREAD_JOIN>", pid, tid_actual);
    log_info(LOGGER_KERNEL, "Se desbloqueara al terminarse el hilo %d", tid_esperado);
    thread_join(pcb, tid_actual, tid_esperado);
}

void syscall_thread_cancel(uint32_t pid, uint32_t tid) {
    if (pid < 0) {
        log_error(LOGGER_KERNEL, "Syscall THREAD_CANCEL: PCB nulo");
        return;
    }

    t_pcb* pcb = obtener_pcb_padre_de_hilo(pid);

    log_info(LOGGER_KERNEL, "Syscall THREAD_CANCEL ejecutada en proceso %d para hilo %d", pcb->PID, tid);
    thread_cancel(pcb, tid);
}

void syscall_thread_exit(uint32_t pid, uint32_t tid) {
    if (pid < 0) {
        log_error(LOGGER_KERNEL, "Syscall THREAD_EXIT: PCB nulo");
        return;
    }

    t_pcb* pcb = obtener_pcb_padre_de_hilo(pid);

    log_info(LOGGER_KERNEL, "Syscall THREAD_EXIT ejecutada por hilo %d en proceso %d", tid, pcb->PID);
    thread_exit(pcb, tid);
}

void syscall_mutex_create(t_tcb* hilo_actual, char* nombre) {
    t_pcb* pcb = obtener_pcb_padre_de_hilo(hilo_actual->PID_PADRE);
    char* mutex = buscar_recurso_proceso(pcb, nombre);

    if(mutex == NULL) {
        t_recurso* recurso_nuevo = malloc(sizeof(t_recurso));
        recurso_nuevo->nombre_recurso = strdup(nombre);
        recurso_nuevo->esta_bloqueado = false;
        recurso_nuevo->hilos_bloqueados = list_create();
        recurso_nuevo->tid_bloqueador = -1;

        list_add(pcb->MUTEXS, nombre);
        list_add(recursos_globales, recurso_nuevo);

        log_info(LOGGER_KERNEL, "Syscall MUTEX_CREATE ejecutada, nuevo mutex creado.");
    } else {
        log_info(LOGGER_KERNEL, "EL mutex %s ya se encuentra creado en el proceso %d", nombre, pcb->PID);
    }

    mover_hilo_a_ready(hilo_actual);

}

void syscall_mutex_lock(t_tcb* hilo_actual, char* nombre) {
    t_recurso* recurso = buscar_recurso_en_global(nombre);

    if(recurso == NULL) {
        log_error(LOGGER_KERNEL, "El mutex %s no existe", nombre);
        return;
    }

    if(recurso->esta_bloqueado) {
        hilo_actual->ESTADO = BLOCK_MUTEX;

        pthread_mutex_lock(&mutex_cola_blocked);
        list_add(cola_blocked_mutex, hilo_actual);
        list_add(recurso->hilos_bloqueados, hilo_actual);
        pthread_mutex_unlock(&mutex_cola_blocked);

        log_warning(LOGGER_KERNEL, "## (<%d>:<%d>) - Bloqueado por <%s>", hilo_actual->PID_PADRE, hilo_actual->TID, recurso->nombre_recurso);
    } else {
        recurso->esta_bloqueado = true;
        recurso->tid_bloqueador = hilo_actual->TID;
        recurso->pid_hilo = hilo_actual->PID_PADRE;

        log_warning(LOGGER_KERNEL, "Mutex %s tomado por (<%d> : <%d>)",recurso->nombre_recurso, recurso->pid_hilo, recurso->tid_bloqueador);

        mover_hilo_a_ready(hilo_actual);
    }
}

void syscall_mutex_unlock(t_tcb* hilo_actual, char* nombre) {
    t_recurso* recurso = buscar_recurso_en_global(nombre);

    if(recurso == NULL) {
        log_error(LOGGER_KERNEL, "El mutex %s no existe", nombre);
        return;
    }

    if(recurso->esta_bloqueado && recurso->tid_bloqueador == hilo_actual->TID && recurso->pid_hilo == hilo_actual->PID_PADRE) {
        recurso->esta_bloqueado = false;
        
        desbloquear_hilo_bloqueado_mutex(recurso);

        ejecutar_hilo(hilo_actual);
    } else {
        // VER BIEN QUE PASA EN ESTE CASO CON EL HILO ACTUAL
        // SI SE BLOQUEA POR EL MUTEX O SI SE LO MANDA A READY DE VUELTA
        log_info(LOGGER_KERNEL, "No se realiza ningun desbloqueo del mutex %s", nombre);
    }
}

char* buscar_recurso_proceso(t_pcb* pcb, char* nombre) {
    for(int i=0; i < list_size(pcb->MUTEXS); i++) {
        char* act = list_get(pcb->MUTEXS, i);
        if(strcmp(act, nombre) == 0) {
            return act;
        }
    }
    return NULL;
}

t_recurso* buscar_recurso_en_global(char* nombre) {
    for(int i=0; i < list_size(recursos_globales); i++) {
        t_recurso* recu = list_get(recursos_globales, i);
        if(strcmp(recu->nombre_recurso, nombre) == 0) {
            return recu;
        }
    }
    return NULL;
}

void desbloquear_hilo_bloqueado_mutex(t_recurso* recurso) {
    if(list_size(recurso->hilos_bloqueados) == 0) {
        log_info(LOGGER_KERNEL, "EL recurso %s no tiene ningun hilo bloqueado", recurso->nombre_recurso);
        return;
    }
    t_tcb* hilo_desbloqueado = list_remove(recurso->hilos_bloqueados, 0);
    syscall_mutex_lock(hilo_desbloqueado, recurso->nombre_recurso);
}

void syscall_dump_memory(uint32_t pid, uint32_t tid) {
    log_info(LOGGER_KERNEL, "Syscall DUMP_MEMORY ejecutandose...");

    t_pcb* pcb = obtener_pcb_padre_de_hilo(pid);

    // Bloquear el hilo actual mientras espera la respuesta del módulo de memoria
    log_info(LOGGER_KERNEL, "Esperando confirmación del módulo de memoria...");

    enviar_memory_dump(pcb, tid);

    sem_wait(&sem_mensaje);

    if(mensaje_okey) {
        log_info(LOGGER_KERNEL, "DUMP_MEMORY completado correctamente para proceso %d, hilo %d", pcb->PID, tid);

        // Mover el hilo a READY
        t_tcb* hilo = buscar_hilo_por_tid(pcb, tid);
        mover_hilo_a_ready(hilo);
    } else {
        log_error(LOGGER_KERNEL, "Error durante la creacion del DUMP_MEMORY");
        log_error(LOGGER_KERNEL, "Se pasa termina la ejecucion del proceso %d y sus hilos", pcb->PID);
        // Mover el proceso a EXIT en caso de error
        //process_exit(pcb);
        process_cancel(pcb);
    }
}

void syscall_io(uint32_t pid, uint32_t tid, int milisegundos) {
    t_pcb* pcb =obtener_pcb_padre_de_hilo(pid);

    if (pcb < 0 || milisegundos <= 0) {
        log_error(LOGGER_KERNEL, "Syscall IO: Argumentos inválidos");
        return;
    }
    log_info(LOGGER_KERNEL, "Syscall IO ejecutada por hilo %d en proceso %d durante %d ms", tid, pcb->PID, milisegundos);
    io(pcb, tid, milisegundos);  // Invoca la funcion que gestiona la operacion IO
}

void log_syscall(char* syscall, t_tcb* tcb) {
    log_info(LOGGER_KERNEL, "## (<%d>:<%d>) - Solicitó syscall: <%s>", tcb->PID_PADRE, tcb->TID, syscall);
}