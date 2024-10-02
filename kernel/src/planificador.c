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

// COLAS QUE EN LAS CUALES SE GUARDARAN LOS PROCESOS
t_list* cola_new;
t_list* cola_ready;
t_list* cola_exit;

// SEMAFOFOS QUE PROTEGEN EL ACCESO A CADA COLA
pthread_mutex_t mutex_cola_new;
pthread_mutex_t mutex_cola_ready;
pthread_mutex_t mutex_cola_exit;

uint32_t pid = 0;
pthread_mutex_t mutex_pid;

void inicializar_colas_y_mutexs() {
    // SE CREA PARA ALMACENAR LOS PROCESOS
    cola_new = list_create();
    cola_ready = list_create();
    cola_exit = list_create();

    // SE INICIALIZAN LOS MUTEX QUE PROTEGERAN LAS COLAS
    pthread_mutex_init(&mutex_cola_new, NULL);
    pthread_mutex_init(&mutex_cola_ready, NULL);
    pthread_mutex_init(&mutex_cola_exit, NULL);
}

t_pcb* crear_pcb(uint32_t pid, uint32_t* tids, t_contexto_ejecucion* contexto_ejecucion, t_estado estado, t_mutex* mutexs){

    t_pcb* pcb = malloc(sizeof(t_pcb));
    if(pcb == NULL) {
        perror("Error al asignar memoria para el PCB");
        exit(EXIT_FAILURE);
    }

    pcb->PID = pid;
    pcb->TIDS = tids;
    pcb->CONTEXTO = contexto_ejecucion;
    pcb->ESTADO = estado;
    pcb->MUTEXS = mutexs;   
    return pcb;

}

t_tcb* crear_tcb(uint32_t tid, int prioridad, t_estado estado){
    t_tcb* tcb = malloc(sizeof(t_tcb));
    if (tcb == NULL) {
        perror("Error al asignar memoria para el TCB");
        exit(EXIT_FAILURE);
    }

    tcb->TID = tid;
    tcb->PRIORIDAD = prioridad;
    //tcb->estado = estado;

    return tcb;
}

// FUNCION QUE CREA UN PROCESO Y LO METE A LA COLA DE NEW
void crear_proceso(char* path_proceso, int tamanio_proceso){
    uint32_t pid = asignar_pid();
    uint32_t* tids = asignar_tids();
    t_contexto_ejecucion* contexto = asignar_contexto();
    pthread_mutex_t* mutexs = asignar_mutexs();
    int prioridad = asignar_prioridad();
    t_pcb* pcb = crear_pcb(pid, tids, contexto, NEW, mutexs);

    pthread_mutex_lock(&mutex_cola_new);
    list_add(cola_new, pcb);
    pthread_mutex_unlock(&mutex_cola_new);

    log_info(LOGGER_KERNEL, "Proceso %d creado en NEW", pcb->PID);

    // Intentar inicializar el proceso en memoria si no hay otros en la cola NEW
    if (list_size(cola_new) == 1) {
        inicializar_proceso(pcb);
    }

    //enviar_proceso_a_memoria(pcb->pid, path_proceso);

}

// ENVIA EL PROCECSO A MEMORIA E INTENTA INICIALIZARLO
void inicializar_proceso(t_pcb* pcb) {
    int resultado = enviar_proceso_a_memoria(pcb->PID, path_procceso);

    if (resultado == EXITO) {
        log_info(logger, "Proceso %d inicializado y movido a READY", pcb->PID);
        mover_a_ready(pcb);
    } else {
        log_warning(logger, "No hay espacio en memoria para el proceso %d , quedara en estado NEW", pcb->PID);
    }
}

void enviar_proceso_a_memoria(int pid_nuevo, char *path_proceso){

    t_paquete* paquete_para_memoria = crear_paquete_codop(codigo_operacion);
    serializar_paquete_para_memoria(paquete_para_memoria, pid_nuevo, path_proceso);
    enviar_paquete(paquete_para_memoria, socket_kernel_memoria); //Poner el socket en el gestor.h
    log_debug(LOGGER_KERNEL, "El PID %d se envio a MEMORIA", pid_nuevo); //Poner el LOGGER en el gestor.h
    eliminar_paquete(paquete_para_memoria);
    
}

void mover_a_ready(t_pcb* pcb) {
    // REMUEVE EL PROCESO DE LA COLA NEW
    pthread_mutex_lock(&mutex_cola_new);
    // LO ELIMINA CUANDO SU PID COINCIDE CON EL DEL PROCESO QUE SE ESTA MOVIENDO
    list_remove_by_condition(cola_new, (void*) (pcb->pid == ((t_pcb*) pcb)->pid)); 
    pthread_mutex_unlock(&mutex_cola_new);

    pcb->ESTADO = READY;
    pthread_mutex_lock(&mutex_cola_ready);
    list_add(cola_ready, pcb);
    pthread_mutex_unlock(&mutex_cola_ready);
    
    log_info(logger, "Proceso %d movido a READY ", pcb->PID);
}

void mover_a_exit(t_pcb* pcb) {
    // REMUEVE EL PROCESO DE LA COLA READY
    pthread_mutex_lock(&mutex_cola_ready);
    list_remove_by_condition(cola_ready, (void*) (pcb->pid == ((t_pcb*) pcb)->pid)); 
    pthread_mutex_unlock(&mutex_cola_ready);

    // PODRIA NECESITARSE QUE SE ELIMINACE DE LA COLA EXEC Y BLOCKED SI ES NECESARIO

    pcb->ESTADO = EXIT;
    pthread_mutex_lock(&mutex_cola_exit);
    list_add(cola_exit, pcb);
    pthread_mutex_unlock(&mutex_cola_exit);
    
    log_info(logger, "Proceso %d movido a EXIT ", pcb->PID);
}

// INTENTA INICIALIZAR EL SIGUIENTE PROCESO EN LA COLA NEW SI HAY UNO DISPONIBLE
void intentar_inicializar_proceso_de_new() {
    pthread_mutex_lock(&mutex_cola_new);
    if (!list_is_empty(cola_new)) {
        t_pcb* pcb = list_remove(cola_new, 0);
        inicializar_proceso(pcb);
    }
    pthread_mutex_unlock(&mutex_cola_new);
}

// MUEVE UN PROCESO A EXIT LIBERANDO SUS RECURSOS E INTENTA INICIALIZAR OTRO A NEW
void process_exit(t_pcb* pcb) {
    mover_a_exit(pcb);
    liberar_recursos_proceso(pcb);
    intentar_inicializar_proceso_de_new();
}

void liberar_recursos_proceso(t_pcb* pcb) {
    if(pcb->TIDS != NULL) {
        free(pcb->TIDS);
    }

    if(pcb->MUTEXS != NULL) {
        free(pcb->MUTEXS);
    }

    free(pcb);

    // No es necesario liberar PID y ESTADO
    // Simplemente son variables que se almacenan en la estructura, y se liberan junto con el PCB.
}