#include <utils/planificador.h>

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

int pid = 0;
pthread_mutex_t mutex_pid;

t_pcb* crearPcb(int pid, int* tids, t_estado estado, int*  mutexs){

    t_pcb* pcb = malloc(sizeof(t_pcb));

    return pcb;
    //preguntar en consultas    
}

t_tcb* crearTcb(){
    t_tcb* tcb = malloc(sizeof(t_tcb));



}

t_pcb crear_proceso(char* path_proceso){
    
    int pid = asignar_pid();
    pthread_mutex_t* mutexs = asignar_mutexs();
    int prioridad = asignar_prioridad();
    t_pcb* pcb = crearPcb(pid, tids, NEW, mutexs, prioridad);

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