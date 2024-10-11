#include <include/syscalls.h>

void syscall_process_create(char* path_proceso, int tamanio_proceso, int prioridad) {
    crear_proceso(path_proceso, tamanio_proceso, prioridad);
    log_info(LOGGER_KERNEL, "Syscall PROCESS_CREATE ejecutada para %s con tamanio %d y prioridad %d", path_proceso, tamanio_proceso, prioridad);
}

void syscall_process_exit(t_pcb* pcb) {
    log_info(LOGGER_KERNEL, "Syscall PROCESS_EXIT ejecutada para proceso %d", pcb->PID);
    process_exit(pcb);
}


void syscall_thread_create(t_pcb* pcb, char* archivo_pseudocodigo, int prioridad){
    log_info(LOGGER_KERNEL, "Syscall THREAD_CREATE ejecutada en proceso %d con prioridad %d", pcb->PID, prioridad);
    thread_create(pcb, archivo_pseudocodigo, prioridad);
}

void syscall_thread_join(t_pcb* pcb, uint32_t tid_actual, uint32_t tid_esperado) {
    log_info(LOGGER_KERNEL, "Syscall THREAD_JOIN ejecutada en proceso %d: hilo %d espera a hilo %d", pcb->PID, tid_actual, tid_esperado);
    thread_join(pcb, tid_actual, tid_esperado);
}

void syscall_thread_cancel(t_pcb* pcb, uint32_t tid) {
    log_info(LOGGER_KERNEL, "Syscall THREAD_CANCEL ejecutada en proceso %d para hilo %d", pcb->PID, tid);
    thread_cancel(pcb, tid);
}

void syscall_thread_exit(t_pcb* pcb, uint32_t tid) {
    log_info(LOGGER_KERNEL, "Syscall THREAD_EXIT ejecutada por hilo %d en proceso %d", tid, pcb->PID);
    thread_exit(pcb, tid);
}

pthread_mutex_t* syscall_mutex_create() {
    pthread_mutex_t* mutex = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mutex, NULL);
    log_info(LOGGER_KERNEL, "Syscall MUTEX_CREATE ejecutada, nuevo mutex creado.");
    return mutex;
}

void syscall_mutex_lock(pthread_mutex_t* mutex) {
    pthread_mutex_lock(mutex);
    log_info(LOGGER_KERNEL, "Syscall MUTEX_LOCK ejecutada.");
}

void syscall_mutex_unlock(pthread_mutex_t* mutex) {
    pthread_mutex_unlock(mutex);
    log_info(LOGGER_KERNEL, "Syscall MUTEX_UNLOCK ejecutada.");
}

void syscall_dump_memory() {
    log_info(LOGGER_KERNEL, "Syscall DUMP_MEMORY ejecutada.");
    dump_memory();  // Esta funcion necesita ser implementada para volcar el estado de la memoria
}

void syscall_io(t_pcb* pcb, uint32_t tid, int milisegundos) {
    log_info(LOGGER_KERNEL, "Syscall IO ejecutada por hilo %d en proceso %d durante %d ms", tid, pcb->PID, milisegundos);
    io(pcb, tid, milisegundos);  // Invoca la funcion que gestiona la operacion IO
}

void manejar_syscall(t_syscall syscall, t_pcb* pcb, char* path_proceso, int prioridad, uint32_t tid_actual, uint32_t tid_esperado, phtread_mutex_t* mutex, int milisegundos) {
    switch (syscall) {
        case PROCESS_CREATE:
            syscall_process_create(path_proceso);
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
            syscall_dump_memory();
            break;
        case IO:
            syscall_io(pcb, tid_actual, milisegundos);
            break;
        default:
            log_error(LOGGER_KERNEL, "Syscall desconocida: %d", syscall);
            break;
    }
}

void dump_memory() {
    log_info(LOGGER_KERNEL, "Dump de memoria:");
    
    // Imprime informacion sobre cada proceso en la memoria
    for (int i = 0; i < list_size(cola_new); i++) {
        t_pcb* proceso = list_get(cola_new, i);
        log_info(LOGGER_KERNEL, "Proceso %d: TIDs: ", proceso->PID);
        
        for (int j = 0; j < CANTIDAD_HILOS; j++) {
            log_info(LOGGER_KERNEL, "\tTID: %d, Estado: %d", proceso->TIDS[j], proceso->ESTADO);
        }
    }
}