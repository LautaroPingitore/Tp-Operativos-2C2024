#include <include/planificador.h>

/*
Tipos de planificacion:
FIFO
Prioridades
Colas multinivel
*/

/*
Instrucciones o sycalls a utilizar

procesos:
PROCESS_CREATE(nombre del archivo pseudocodigo que ejecutara el proceso, tamaño del proceso, prioridad del hilo)
PROCESS_ EXIT()

threads:
THREAD_CREATE(nombre del archivo pseudocodigo que debera ejecutar el hilo, prioridad)
THREAD_JOIN(TID)
THREAD_CANCEL(TID)
THREAD_EXIT()

mutex:
MUTEX_CREATE()
MUTEX_LOCK()
MUTEX_UNLOCK()

memoria:
DUMP_MEMORY()

entrada y salida:
IO(cantidad de milisegundos que el hilo va a permanecer haciendo la operación de entrada/salida)
*/


//PLANIFICADOR LARGO PLAZO

uint32_t pid = 0;
pthread_mutex_t mutex_pid;

t_pcb* crearPcb(uint32_t pid, uint32_t* tids, t_contexto_ejecucion* contexto_ejecucion, t_estado estado, t_mutex* mutexs){

    t_pcb* pcb = malloc(sizeof(t_pcb));
    if(pcb == NULL) {
        perror("Error al asignar memoria para el PCB");
        exit(EXIT_FAILURE);
    }

    pcb->pid = pid;
    pcb->tid = tids;
    pcb->contexto_ejecucion = contexto_ejecucion;
    pcb->estado = estado;
    pcb->MUTEXS = mutexs;   
    return pcb;

}

t_tcb* crearTcb(uint32_t tid, uint32_t prioridad, t_estado estado){
    t_tcb* tcb = malloc(sizeof(t_tcb));
    if (tcb == NULL) {
        perror("Error al asignar memoria para el TCB");
        exit(EXIT_FAILURE);
    }

    tcb->tid = tid;
    tcb->prioridad = prioridad;
    tcb->estado = estado;

    return tcb;
}

t_pcb crear_proceso(char* path_proceso, int tamanio_proceso){
    
    uint32_t pid = asignar_pid();
    pthread_mutex_t* mutexs = asignar_mutexs();
    int prioridad = asignar_prioridad();
    t_pcb* pcb = crearPcb(pid, tids, contexto, NEW, mutexs);

    pthread_mutex_lock(&pcbs_de_procesos_en_sistema_mutex);
    list_add(pcbs_de_procesos_en_sistema, pcb);
    pthread_mutex_unlock(&pcbs_de_procesos_en_sistema_mutex);

    log_info(LOGGER_KERNEL, "Proceso %d creado en NEW", pcb->PID);

    enviar_proceso_a_memoria(pcb->pid, path_proceso);

}


void enviar_proceso_a_memoria(int pid_nuevo, char *path_proceso){

    t_paquete* paquete_para_memoria = crear_paquete_codop(codigo_operacion);
    serializar_paquete_para_memoria(paquete_para_memoria, pid_nuevo, path_proceso);
    enviar_paquete(paquete_para_memoria, socket_kernel_memoria); //Poner el socket en el gestor.h
    log_debug(LOGGER_KERNEL, "El PID %d se envio a MEMORIA", pid_nuevo); //Poner el LOGGER en el gestor.h
    eliminar_paquete(paquete_para_memoria);
    
}