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
int sys_thread_create(char* path, int priority);
int sys_thread_join(int tid);
int sys_thread_cancel(int tid);
void sys_thread_exit();
void sys_io(int milliseconds);


void syscall_process_create(char*, int, int);
void syscall_process_exit(t_pcb*);
void syscall_thread_create(t_pcb*, char*, int);
void syscall_thread_join(t_pcb*, uint32_t, uint32_t);
void syscall_thread_cancel(t_pcb*, uint32_t);
void syscall_thread_exit(t_pcb*, uint32_t);
pthread_mutex_t* syscall_mutex_create();
void syscall_mutex_lock(pthread_mutex_t*);
void syscall_mutex_unlock(pthread_mutex_t*);
void syscall_dump_memory(t_pcb*, uint32_t);
void syscall_io(t_pcb*, uint32_t, int);
void manejar_syscall(op_code, t_pcb*, char*, int, int, uint32_t, uint32_t, pthread_mutex_t*, int);

#endif /* SYSCALL_H_ */