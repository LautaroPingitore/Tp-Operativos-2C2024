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
int CANTIDAD_HILOS = 3; // DE MOMENTO PUSIMOS QUE UN PROCESO TIENE 3 HILOS, CUIDADO CON ESTO
int CANTIDAD_MUTEX = 3; // LO MISMO QUE LOS HILOS 

// SEMAFOFOS QUE PROTEGEN EL ACCESO A CADA COLA
pthread_mutex_t mutex_cola_new;
pthread_mutex_t mutex_cola_ready;
pthread_mutex_t mutex_cola_exit;

uint32_t pid = 0;
uint32_t contador_tid = 0;
pthread_mutex_t mutex_pid = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_tid = PTHREAD_MUTEX_INITIALIZER;

bool cpu_libre = true;

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

t_pcb* crear_pcb(uint32_t pid, uint32_t* tids, t_contexto_ejecucion* contexto_ejecucion, t_estado estado, pthread_mutex_t* mutexs){

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
void crear_proceso(char* path_proceso, int tamanio_proceso, int prioridad){
    uint32_t pid = asignar_pid();
    t_contexto_ejecucion* contexto = inicializar_contexto();
    pthread_mutex_t* mutexs = asignar_mutexs();
    t_pcb* pcb = crear_pcb(pid, [], contexto, NEW, mutexs);

    thread_create(pcb, path_proceso, prioridad);

    pthread_mutex_lock(&mutex_cola_new);
    list_add(cola_new, pcb);
    pthread_mutex_unlock(&mutex_cola_new);

    log_info(LOGGER_KERNEL, "Proceso %d creado en NEW", pcb->PID);

    // Intentar inicializar el proceso en memoria si no hay otros en la cola NEW
    if (list_size(cola_new) == 1) {
        inicializar_proceso(pcb, path_proceso);
    }

    enviar_proceso_a_memoria(pcb->pid, path_proceso);

}

uint32_t asignar_pid() {
    pthread_mutex_lock(&mutex_pid);
    uint32_t pidNuevo = pid;
    pid++;
    pthread_mutex_unlock(&mutex_pid);

    return pidNuevo;
}

uint32_t asignar_tids() {
    // Asignamos TIDs unicos para cada hilo del proceso
    pthread_mutex_lock(&mutex_tid);  // Protegemos el acceso a contador_tid con un mutex    
    uint32_t nuevo_tid = contador_tid;
    contador_tid++;
    pthread_mutex_unlock(&mutex_tid);

    return nuevo_tid;
} 

t_contexto_ejecucion* inicializar_contexto() {
    t_contexto_ejecucion* contexto = malloc(sizeof(t_contexto_ejecucion));
    if (contexto == NULL) {
        perror("Error al asignar memoria para el contexto de ejecucion");
        exit(EXIT_FAILURE);
    }
    
    // Inicializacion de registros y estado del proceso
    contexto->registros = malloc(sizeof(t_registros));
    memset(contexto->registros, 0, sizeof(t_registros));
    contexto->registros->program_counter = 0;
    
    contexto->motivo_desalojo = ESTADO_INICIAL;
    contexto->motivo_finalizacion = INCIAL;

    return contexto;
}

// CUIDADO, EXISTE UN STRUCT DE MUTEX EN LA BIBLIOTECA
// pthread_mutex_t 
pthread_mutex_t* asignar_mutexs() {
   pthread_mutex_t* mutexs = malloc(sizeof(pthread_mutex_t) * CANTIDAD_MUTEX);
    if (mutexs == NULL) {
        perror("Error al asignar memoria para los mutex");
        exit(EXIT_FAILURE);
    }

    // Inicializacion de cada mutex
    for (int i = 0; i < CANTIDAD_MUTEX; i++) {
        pthread_mutex_init(&mutexs[i], NULL);
    }

    return mutexs;
}


// ENVIA EL PROCECSO A MEMORIA E INTENTA INICIALIZARLO
void inicializar_proceso(t_pcb* pcb, char* path_proceso) {
    // ARREGLAR ESTA FUNCION
    int resultado = enviar_proceso_a_memoria(pcb->PID, path_proceso);

    if (resultado != -1) {
        log_info(logger, "Proceso %d inicializado y movido a READY", pcb->PID);
        mover_a_ready(pcb);
    } else {
        log_warning(logger, "No hay espacio en memoria para el proceso %d , quedara en estado NEW", pcb->PID);
    }
}

int enviar_proceso_a_memoria(int pid_nuevo, char *path_proceso){

    t_paquete* paquete_para_memoria = crear_paquete_codop();//(codigo_operacion)
    serializar_paquete_para_memoria(paquete_para_memoria, pid_nuevo, path_proceso);

    int resultado = enviar_paquete(paquete_para_memoria, socket_kernel_memoria); //Poner el socket en el gestor.h
    if(resultado == -1){
        eliminar_paquete(paquete_para_memoria);
        return resultado;    
    }
    
    log_info(LOGGER_KERNEL, "El PID %d se envio a MEMORIA", pid_nuevo); //Poner el LOGGER en el gestor.h
    eliminar_paquete(paquete_para_memoria);
    
    return resultado;
}

void serializar_paquete_para_memoria(t_paquete* paquete, int pid, char* path_proceso) {
    int size_pid = sizeof(int);
    agregar_a_paquete(paquete, &pid_nuevo, size_pid);

    int length_path = strlen(path_proceso) + 1;
    agregar_a_paquete(paquete, &length_path, sizeof(int));
    agregar_a_paquete(paquete, path_proceso, length_path);
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
        t_pcb* pcb = list_remove(cola_new, 0); // Guarda con lo que retorna list_remove
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

    cpu_libre = true;

    free(pcb);

    // No es necesario liberar PID y ESTADO
    // Simplemente son variables que se almacenan en la estructura, y se liberan junto con el PCB.
}


// MANEJO DE HILOS ==============================

// CREA UN NUEVO TCB ASOSICADO AL PROCESO Y LO CONFIGURA CON EL PSEUDOCODIGO A EJECUTAR
void thread_create(t_pcb *pcb, char* archivo_pseudocodigo, int prioridad) {

    uint32_t nuevo_tid = asigar_tid();

    // CREA EL NUEVO HILO
    t_tcb* nuevo_tcb = crear_tcb(nuevo_tid, prioridad, NEW);
    list_add(pcb->TIDS, nuevo_tid);

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

//                         CUIDADO
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
        esperar_a_que_termine(tcb_esperado, tcb_actual);
    }
}

void esperar_a_que_termine(t_tcb* esperado,  t_tcb* actual) {
    bloquear_hilo_actual(actual);

    while(tcb_esperado->ESTADO != EXIT) {
        usleep(100);
    }

    desbloquear_hilo_actual(actual);
}

void bloquear_hilo_actual(t_tcb* tcb_actual) {

    // Cambiar el estado del hilo a BLOCK
    tcb_actual->ESTADO = BLOCK;

    log_info(LOGGER_KERNEL, "Hilo %d bloqueado.", tcb_actual->TID);

    pthread_yield();  // Permite que otros hilos se ejecuten
}

void desbloquear_hilo_actual(t_tcb* tcb_actual) {

    // Cambiar el estado del hilo a READY
    tcb_actual->ESTADO = READY;

    // Aniadir el hilo a la cola de READY
    pthread_mutex_lock(&mutex_cola_ready);
    list_add(cola_ready, tcb_actual);  // Mover el hilo a la cola de READY
    pthread_mutex_unlock(&mutex_cola_ready);

    log_info(LOGGER_KERNEL, "Hilo %d desbloqueado y movido a READY.", tcb_actual->TID);
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

    intentar_mover_a_execute(); //Intenta a mover a execute el proximo hilo, no el que finalizo
}

// A CHEQUEAR
void intentar_mover_a_execute() {
    // Verificar si hay algun proceso en READY y si la CPU esta libre
    pthread_mutex_lock(&mutex_cola_ready);
    if (list_is_empty(cola_ready)) {
        log_info(logger, "No hay procesos en READY para mover a EXECUTE");
        return;
    }
    pthread_mutex_unlock(&mutex_cola_ready);

    if (!cpu_libre) {
        log_info(logger, "CPU ocupada, no se puede mover un proceso a EXECUTE");
        return;
    }

    // Obtener el primer proceso de la cola de READY
    pthread_mutex_lock(&mutex_cola_ready);
    t_pcb* proceso_seleccionado = list_remove(cola_ready, 0);
    pthread_mutex_lock(&mutex_cola_ready);

    if (proceso_seleccionado == NULL) {
        log_warning(logger, "Error al obtener un proceso de la cola READY");
        return;
    }

    // Cambiar el estado del proceso a EXECUTE
    proceso_seleccionado->ESTADO = EXECUTE;

    cpu_libre = false;

    // Loggear la operacion
    log_info(logger, "Proceso %d movido a EXECUTE", proceso_seleccionado->PID);

    // Enviar el proceso a la CPU para su ejecucion

    int resultado = enviar_proceso_a_cpu(proceso_seleccionado);

    if (resultado == 0) {
        log_info(logger, "El proceso %d ha sido enviado a la CPU", proceso_seleccionado->PID);
    } else {
        log_error(logger, "Error al enviar el proceso %d a la CPU", proceso_seleccionado->PID);
        // Si hubo un error, devolver el proceso a READY
        proceso_seleccionado->ESTADO = READY;
        cpu_libre = true;
        pthread_mutex_lock(&mutex_cola_new);
        list_add(cola_ready, proceso_seleccionado);
        pthread_mutex_lock(&mutex_cola_new);
    }
}


// PLANIFICADOR A CORTO PLAZO ===============

// ESTA FUNCION RETORNA EL HILO QUE TOCA POR FIFO Y LO SACA DE READY
t_tcb* obtener_hilo_fifo() {
    pthread_mutex_lock(&mutex_cola_ready);

    if(list_is_empty(cola_ready)) { // SI NO HAY HILOS EN LA COLA NO RETORNA NULL
        pthread_mutex_unlock(&mutex_cola_ready);
        return NULL;
    }

    t_tcb* hilo_a_ejecutar = list_remove(cola_ready, 0); // OBTIENE EL PRIMER HILO DE LA COLA
    pthread_mutex_unlock(&mutex_cola_ready);

    return hilo_a_ejecutar;
}

//ESTA FUNCION RETORNA EL HILO CON MAYOR PRIORIDAD DE LA COLA READY Y LO SACA DE LA COLA
t_tcb* obtener_hilo_x_prioridad() {
    pthread_mutex_lock(&mutex_cola_ready);

    if(list_is_empty(cola_ready)) { // SI NO HAY HILOS EN LA COLA NO RETORNA NULL
        pthread_mutex_unlock(&mutex_cola_ready);
        return NULL;
    }

    t_tcb* hilo_a_ejecutar = NULL;
    int prioridad_max = INT_MAX; // INICIALIZA EN UN VALOR GRANDE ASI SIEMPRE PASA 1 VEZ POR EL IF DEL FOR

    for(int i=0; i < list_size(cola_ready); i++) {
        t_tcb* hilo_actual = list_get(cola_ready, i); // OBTIENE EL HILO EN LA POSICION i DE LA COLA
        if(hilo_actual->PRIORIDAD < prioridad_max) { // VERIFICA SI EL HILO ACTUAL TIENE MAS PRIORIDAD QUE EL QUE ESTA GUARDADO
            prioridad_max = hilo_actual -> PRIORIDAD;
            hilo_a_ejecutar = hilo_actual;
        }
    }

    list_remove_by_condition(cola_ready, (void*) (hilo_mayor_prioridad->TID == ((t_tcb*) hilo_mayor_prioridad)->TID)); // SACA EL HILO DE LA COLA

    pthread_mutex_unlock(&mutex_cola_ready);

    return hilo_a_ejecutar;
}

t_list* cola_nivel_1;  // Mayor prioridad
t_list* cola_nivel_2;  // Prioridad media
t_list* cola_nivel_3;  // Menor prioridad

t_tcb* seleccionar_hilo_multinivel() {
    pthread_mutex_lock(&mutex_cola_ready);
    t_tcb* siguiente_hilo = NULL;

    // Revisamos la cola de mayor prioridad primero
    if (!list_is_empty(cola_nivel_1)) {
        siguiente_hilo = list_remove(cola_nivel_1, 0);  // Removemos el primer hilo de la cola 1
    } else if (!list_is_empty(cola_nivel_2)) {
        siguiente_hilo = list_remove(cola_nivel_2, 0);  // Removemos el primer hilo de la cola 2
    } else if (!list_is_empty(cola_nivel_3)) {
        siguiente_hilo = list_remove(cola_nivel_3, 0);  // Removemos el primer hilo de la cola 3
    }

    pthread_mutex_unlock(&mutex_cola_ready);

    return siguiente_hilo;  // Este hilo sera movido a EXECUTE
}

// ENTRADA Y SALIDA ====================

void io(t_pcb* pcb, uint32_t tid, int milisegundos) {
    t_tcb* tcb = buscar_hilo_por_tid(pcb, tid);
    if(tcb == NULL) {
        log_error(LOGGER_KERNEL, "Error: Hilo con TID %d no encontrado en el proceso %d", tid, pcb->PID);
        return;
    }

    tcb->ESTADO = BLOCK;
    log_info(LOGGER_KERNEL, "Hilo %d en proceso %d ha iniciado IO por %d ms", tid, pcb->PID, milisegundos);

    usleep(milisegundos * 1000);

    tcb->ESTADO = READY;
    phtread_mutex_lock(&mutex_cola_ready);
    list_add(cola_ready, tcb);
    phtread_mutex_unlock(&mutex_cola_ready);

    log_info(LOGGER_KERNEL, "Hilo %d en proceso %d ha terminado IO y esta en READY", tid, pcb->PID);
}

