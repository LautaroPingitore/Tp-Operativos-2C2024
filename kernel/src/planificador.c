#include <include/planificador.h>

//AGREGO ESTO PARA SOLUCIONAR PROBLEMA PERO PUEDE SACARSE SI HAY OTRA FORMA
t_log* LOGGER_KERNEL;
int socket_kernel_memoria;

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


//PLANIFICADOR LARGO PLAZO ==============================

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
    tcb->ESTADO = estado;

    return tcb;
}

// FUNCION QUE CREA UN PROCESO Y LO METE A LA COLA DE NEW
void crear_proceso(char* path_proceso, int tamanio_proceso){
    uint32_t pid = asignar_pid();
    uint32_t* tids = asignar_tids();
    t_contexto_ejecucion* contexto = asignar_contexto();
    t_mutex* mutexs = asignar_mutexs();
    // int prioridad = asignar_prioridad();
    t_pcb* pcb = crear_pcb(pid, tids, contexto, NEW, mutexs);

    pthread_mutex_lock(&mutex_cola_new);
    list_add(cola_new, pcb);
    pthread_mutex_unlock(&mutex_cola_new);

    log_info(LOGGER_KERNEL, "Proceso %d creado en NEW", pcb->PID);

    // Intentar inicializar el proceso en memoria si no hay otros en la cola NEW
    if (list_size(cola_new) == 1) {
        inicializar_proceso(pcb, path_proceso);
    }

    //enviar_proceso_a_memoria(pcb->pid, path_proceso);

}

uint32_t asignar_pid() {

}
uint32_t* asignar_tids() {

}
t_contexto_ejecucion* asignar_contexto() {

}
// CUIDADO, EXISTE UN STRUCT DE MUTEX EN LA BIBLIOTECA
// pthread_mutex_t 
t_mutex* asignar_mutexs() {

}
int asignar_prioridad() {

}

// ENVIA EL PROCECSO A MEMORIA E INTENTA INICIALIZARLO
void inicializar_proceso(t_pcb* pcb, char* path_proceso) {
    // ARREGLAR ESTA FUNCION
    /*int resultado = enviar_proceso_a_memoria(pcb->PID, path_proceso);

    if (resultado == EXITO) {
        log_info(logger, "Proceso %d inicializado y movido a READY", pcb->PID);
        mover_a_ready(pcb);
    } else {
        log_warning(logger, "No hay espacio en memoria para el proceso %d , quedara en estado NEW", pcb->PID);
    }
    */
}

void enviar_proceso_a_memoria(int pid_nuevo, char *path_proceso){

    t_paquete* paquete_para_memoria = crear_paquete_codop();//(codigo_operacion)
    serializar_paquete_para_memoria(paquete_para_memoria, pid_nuevo, path_proceso);
    enviar_paquete(paquete_para_memoria, socket_kernel_memoria); //Poner el socket en el gestor.h
    log_debug(LOGGER_KERNEL, "El PID %d se envio a MEMORIA", pid_nuevo); //Poner el LOGGER en el gestor.h
    eliminar_paquete(paquete_para_memoria);
    
}

void mover_a_ready(t_pcb* pcb) {
    // REMUEVE EL PROCESO DE LA COLA NEW
    pthread_mutex_lock(&mutex_cola_new);
    // LO ELIMINA CUANDO SU PID COINCIDE CON EL DEL PROCESO QUE SE ESTA MOVIENDO
    list_remove_by_condition(cola_new, (void*) (pcb->PID == ((t_pcb*) pcb)->PID)); 
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
    list_remove_by_condition(cola_ready, (void*) (pcb->PID == ((t_pcb*) pcb)->PID)); 
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


// MANEJO DE HILOS ==============================

pthread_mutex_t mutex_tid = PTHREAD_MUTEX_INITIALIZER;
uint32_t contador_tid = 0; // PUEDE USARSE EL STATIC DENTRO DE LA FUNCION CREATE


// CREA UN NUEVO TCB ASOSICADO AL PROCESO Y LO CONFIGURA CON EL PSEUDOCODIGO A EJECUTAR
void thread_create(t_pcb *pcb, char* archivo_pseudocodigo, int prioridad) {

    //static uint32_t contador_tid = 0; 
    // VARIABLE QUE NO SE REINICIA SI SE LLAMA DE NUEVO A LA FUNCION (VARIABLE GLOBAL)

    // ASIGNA UN TID UNICO
    pthread_mutex_lock(&mutex_tid);
    uint32_t nuevo_tid = ++contador_tid;
    pthread_mutex_unlock(&mutex_tid);

    // CREA EL NUEVO HILO
    t_tcb* nuevo_tcb = crear_tcb(nuevo_tid, prioridad, NEW);
    // ASIGNA EL HILO AL PROCESO
    list_add(pcb->TIDS, nuevo_tcb);

    // CARGA PSEUDOGODIO A EJECUTAR
    cargar_archivo_pseudocodigo(nuevo_tcb, archivo_pseudocodigo);

    // MUEVE EL HILO A LA COLA DE READY SI PUEDE
    pthread_mutex_lock(&mutex_cola_ready);
    list_add(cola_ready, nuevo_tcb);
    pthread_mutex_unlock(&mutex_cola_ready);

    log_info(LOGGER_KERNEL, "Hilo %d creado en el proceso %d", nuevo_tid, pcb->PID);
}

void cargar_archivo_pseudocodigo(t_tcb* tcb, char* archivo_pseudocodigo) {

}


// HACE QUE UN HILO ESPERE LA FINALIZACION DE OTRO HILO DEL MISMO PROCESO
// PODRIA USAR UN SEMAFORO DE MUTEX COLAS PERO NO ESTOY SEGURO
void thread_join(t_pcb* pcb, uint32_t tid_actual, uint32_t tid_esperado) {

    // BUSCA EL HILO ESPERADO EN EL PCB
    t_tcb* tcb_esperado = buscar_hilo_por_tid(pcb, tid_esperado);
    if (tcb_esperado == NULL) {
        log_error(LOGGER_KERNEL, "Error: TID %d no encontrado en el proceso %d", tid_esperado, pcb->PID);
        return;
    }

    t_tcb* tcb_actual = buscar_hilo_por_tid(pcb, tid_actual);
    if (tcb_actual == NULL) {
        log_info(LOGGER_KERNEL, "Error: No se encontro el hilo actual  %d", tid_actual);
        return;
    }

    // VE SI EL HILO ESTA EJECUTANDOSE O EN READY, EN DICHO CASO BLOQUEA EL HILO ACTUAL
    if (tcb_esperado->ESTADO == EXIT) {
        log_info(LOGGER_KERNEL, "El hilo %d ya terminado, no es necesario hacer el join", tid_esperado);
    } else {
        log_info(LOGGER_KERNEL, "EL hilo %d esperara al hilo %d", tid_actual, tid_esperado);
        bloquear_hilo_actual(tid_actual); // BLOQUEA EL HILO ACTUAL HASTA QUE TERMINE EL HILO ESPERADO
    }
}

t_tcb* buscar_hilo_por_tid(t_pcb* pcb, uint32_t tid) {
    for (int i=0; i < list_size(pcb->TIDS); i++) {
        t_tcb tcb = list_get(pcb->TIDS, i);
        if (tcb->TID == tid) {
            return tcb;
        }
    }
    return NULL;
}


// FINALIZA LA EJECUCION DE UN HILO ANTES DE QUE TERMINE
void thread_cancel(t_pcb* pcb, uint32_t tid) {

    t_tcb* tcb = buscar_hilo_por_tid(pcb, tid);

    if (tcb == NULL) {
        log_error(LOGGER_KERNEL, "Error: TID %d no encontrado en el proceso %d", tid, pcb->PID);
        return;
    }

    // CAMBIA EL ESTADO DEL TCB Y LIBERA LOS RECURSOS
    tcb->ESTADO = EXIT;
    liberar_recursos_hilo(tcb);
    log_info(LOGGER_KERNEL, "Hilo %d cancelado en el proceso %d", tid, pcb->PID);

    // PUEDE HABER UNA FUNCION QUE MUEVA OTRO A EXECUTE SI ES NECESARIO
    // intentar_mover_a_execute();
}

void liberar_recursos_hilo(t_tcb* tcb) {
    free(tcb->TID); // NO SE SI SERA NECESARIO ESTO O SI FALTA ALGO
    free(tcb);
}


// UN HILO FINALIZA SU EJECUCION POR SU PROPIA CUENTA
void thread_exit(t_pcb* pcb, uint32_t tid) {
    t_tcb* tcb = buscar_hilo_por_tid(pcb, tid);

    if (tcb == NULL) {
        log_error(LOGGER_KERNEL, "Error: TID %d no encontrado en el proceso %d", tid, pcb->PID);
        return;
    }

    // MARCA EL HILO COMO FINALIZADO
    tcb->ESTADO = EXIT;
    liberar_recursos_hilo(tcb);
    log_info(LOGGER_KERNEL, "Hilo %d ha finalizado en el proceso %d", tid, pcb->PID);

    intentar_mover_a_execute();
}

void intentar_mover_a_execute() {

}