#ifndef SYSCALL_H_
#define SYSCALL_H_

// Definicion de codigos de operacion para las syscalls
// #define SYS_THREAD_CREATE 1
// #define SYS_THREAD_JOIN 2
// #define SYS_THREAD_CANCEL 3
// #define SYS_THREAD_EXIT 4
// #define SYS_IO 5

// typedef enum {
//     PROCESS_CREATE,
//     PROCESS_EXIT,
//     THREAD_CREATE,
//     THREAD_JOIN,
//     THREAD_CANCEL,
//     THREAD_EXIT,
//     MUTEX_CREATE,
//     MUTEX_LOCK,
//     MUTEX_UNLOCK,
//     DUMP_MEMORY,
//     IO
// } t_syscall;

// Prototipos de funciones syscalls
void syscall_process_create(t_tcb*, char*, int, int);
void syscall_process_exit(uint32_t);
void syscall_thread_create(t_tcb*, uint32_t, char*, int);
void syscall_thread_join(uint32_t, uint32_t, uint32_t);
void syscall_thread_cancel(uint32_t, uint32_t);
void syscall_thread_exit(uint32_t, uint32_t);
void syscall_mutex_create(t_tcb*, char*);
void syscall_mutex_lock(t_tcb*, char*);
void syscall_mutex_unlock(t_tcb*, char*);
void syscall_dump_memory(uint32_t, uint32_t);
void syscall_io(uint32_t, uint32_t, int);
void log_syscall(char*, t_tcb*);
char* buscar_recurso_proceso(t_pcb*, char*);
t_recurso* buscar_recurso_en_global(char*);
void desbloquear_hilo_bloqueado_mutex(t_recurso*);
void reiniciar_quantum();

#endif /* SYSCALL_H_ */