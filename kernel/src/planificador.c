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
    pcb->PID = asignar_pid(pcb);
    pcb->ESTADO = NEW;
    pcb->TIDS = crear_lista_tids();
    pcb->MUTEXS = crear_lista_mutex();
    
    return pcb;
    //preguntar en consultas    
}

t_tcb* crearTcb(){
    t_tcb* tcb = malloc(sizeof(t_tcb));
    tcb->TID = asignar_tid();
    tcb->PRIORIDAD = asignar_prioridad();
}


