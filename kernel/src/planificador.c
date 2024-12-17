#include "include/gestor.h"

/*
=== PASO A PASO DE LA EJECUCION === 
1) Al iniciar kernel se crea un proceso inicial y un hilo principal para dicho proceso
donde el archivo de pseudocodigo del hilo 0 es el mismo que del proceso 
2) Al crearce un proceso este pasa a la cola NEW y se le manda un pedido a memoria para
inicializar el mismo, en caso de que no haya espacio, este se queda en esta cola
3) Al inicializarce un proceso, este pasa a la cola READY y se elige el hilo a ejecutar
en base al algoritmo de planificacion que se este usando
4) Una vez que se elige el hilo a ejecutar se lo pasa a la cola EXEC y se envia a CPU
el TID y su PID asociado a travez de DISPATCH quedando a la espera de recibir
otra vez el TID con el motivo de devolucion
5) AL recibir el TID del hilo en ejecucion, si dicho motivo implica replanificar
se seleccionara el siguiente hilo a ejecutar (PASO 3), durante este periodo la CPU
queda esperando
6) SYSCALLS: Esto podria estar metido dentro de CPU dentro de la funcion que maneja
las instrucciones como caso aparte donde si ocurre una syscall se debe bloquear 
al hilo ejectandose y devolver el control al kernel para que este vuelva a planificar todo (PASO 3)

=== CONEXIONES USADAS ===
- CPU: Una vez que se selecciono el hilo a ejecutar se le debe mandar a CPU
       el TID y el PID padre asociado al hilo a travez de DISPATCH

       Una vez que el hilo termino de ejecutarse (DISPATCH) o ocurrio 
       una interrupcion (INTERRUPT), CPU le manda a KERNEL el TID para que
       este elija el proximo hilo a ejecutar

- MEMORIA: Al crear un proceso se le envia a MEMORIA el PCB para ver si puede
           inicializarce o no el proceso

           AL finalizar un proceso se le envia a MEMORIA el PCB para que este
           lo libere de la memoria

           Al crear un hilo, KERNEL le debe mandar a MEMORIA el TCB

           Al finalizar un hilo KERNEL le debe mandar a MEMORIA el TCB, liberar
           el TCB asociado y mover a READY todos los hilos bloqueados por ese TID

*/


//PLANIFICADOR LARGO PLAZO ==============================
    
// COLAS QUE EN LAS CUALES SE GUARDARAN LOS PROCESOS
t_list* cola_new;
t_list* cola_ready;
t_list* cola_exec;
t_list* cola_blocked_join;
t_list* cola_blocked_mutex;
t_list* cola_blocked_io;
t_list* cola_exit;
t_list* colas_multinivel;

t_list* tabla_paths;
t_list* tabla_procesos;
t_list* recursos_globales;
// uint32_t pid_actual = 0;
// uint32_t tid_actual = 0;

// SEMAFOFOS QUE PROTEGEN EL ACCESO A CADA COLA
pthread_mutex_t mutex_cola_new;
pthread_mutex_t mutex_cola_ready;
pthread_mutex_t mutex_cola_exit;
pthread_mutex_t mutex_cola_blocked;
pthread_mutex_t mutex_cola_exec;
pthread_mutex_t mutex_pid;
pthread_mutex_t mutex_tid;
pthread_mutex_t mutex_estado;
pthread_mutex_t mutex_join;
pthread_mutex_t mutex_cola_multinivel;

sem_t sem_io;

// VARIABLES DE CONTROL
uint32_t pid = 0;
uint32_t contador_tid = 0;
bool cpu_libre = true;

void inicializar_kernel() {
    cola_new = list_create();
    cola_exec = list_create();
    cola_exit = list_create();
    cola_blocked_join = list_create();
    cola_blocked_mutex = list_create();
    cola_blocked_io = list_create();

    if(strcmp(ALGORITMO_PLANIFICACION, "CMN") == 0) {
        colas_multinivel = list_create();
    } else {
        cola_ready = list_create();
    }

    tabla_paths = list_create();
    tabla_procesos = list_create();
    recursos_globales = list_create();

    pthread_mutex_init(&mutex_pid, NULL);
    pthread_mutex_init(&mutex_tid,NULL);
    pthread_mutex_init(&mutex_estado, NULL);
    pthread_mutex_init(&mutex_cola_new, NULL);
    pthread_mutex_init(&mutex_cola_ready, NULL);
    pthread_mutex_init(&mutex_cola_exit, NULL);
    pthread_mutex_init(&mutex_cola_blocked, NULL);
    pthread_mutex_init(&mutex_mensaje, NULL);
    pthread_mutex_init(&mutex_cola_exec, NULL);
    pthread_mutex_init(&mutex_cola_multinivel, NULL);
    pthread_mutex_init(&mutex_join, NULL);

    sem_init(&sem_mensaje, 0, 0);
    sem_init(&sem_io, 0, 0);
}

uint32_t asignar_pid() {
    pthread_mutex_lock(&mutex_pid);
    uint32_t pidNuevo = pid;
    pid++;
    pthread_mutex_unlock(&mutex_pid);

    return pidNuevo;
}

uint32_t asignar_tid(t_pcb* pcb) {
    pthread_mutex_lock(&mutex_tid); 
    uint32_t nuevo_tid = list_size(pcb->TIDS);
    pthread_mutex_unlock(&mutex_tid);

    return nuevo_tid;
} 

t_pcb* crear_pcb(uint32_t pid, int tamanio, t_contexto_ejecucion* contexto_ejecucion, t_estado estado, char* archivo){

    t_pcb* pcb = malloc(sizeof(t_pcb));
    if(pcb == NULL) {
        log_error(LOGGER_KERNEL, "Error al asignar memoria para el PCB");
        exit(EXIT_FAILURE);
    }

    pcb->PID = pid;
    pcb->TIDS = list_create();
    pcb->TAMANIO = tamanio;
    pcb->CONTEXTO = contexto_ejecucion;
    pcb->ESTADO = estado;
    pcb->MUTEXS = list_create();
    
    // ACA PODRIAMOS AGREGAR EL ARCHIVO A LA TABLA DE PATHS
    // EN LA POSICION DEL PID
    agregar_path(pid, archivo);
    list_add(tabla_procesos, pcb);

    t_tcb* hilo_principal = crear_tcb(pid, asignar_tid(pcb), archivo, 0, NEW);

    list_add(pcb->TIDS, hilo_principal);
    // mover_hilo_a_ready(hilo_principal);
    envio_hilo_crear(socket_kernel_memoria, hilo_principal, THREAD_CREATE);

    return pcb;
}

t_tcb* crear_tcb(uint32_t pid_padre, uint32_t tid, char* archivo_pseudocodigo, int prioridad, t_estado estado){
    t_tcb* tcb = malloc(sizeof(t_tcb));
    if (tcb == NULL) {
        log_error(LOGGER_KERNEL, "Error al asignar memoria para el TCB");
        exit(EXIT_FAILURE);
    }

    tcb->PID_PADRE = pid_padre;
    tcb->TID = tid;
    tcb->PRIORIDAD = prioridad;
    tcb->ESTADO = estado;
    tcb->archivo = archivo_pseudocodigo;
    tcb->PC = 0;
    tcb->motivo_desalojo = ESTADO_INICIAL;

    return tcb;
}

// FUNCION QUE CREA UN PROCESO Y LO METE A LA COLA DE NEW
void crear_proceso(char* path_proceso, int tamanio_proceso, int prioridad){
    //archivo_pseudocodigo* archivo = leer_archivo_pseudocodigo(path_proceso);

    t_pcb* pcb = crear_pcb(asignar_pid(), tamanio_proceso, inicializar_contexto(), NEW, path_proceso);

    //obtener_recursos_del_proceso(archivo, pcb);
    
    // EN LA LISTA DE NEW, READY, ETC TENDRIAN QUE SER HILOS, NO PROCESOS
    pthread_mutex_lock(&mutex_cola_new);
    list_add(cola_new, pcb);
    pthread_mutex_unlock(&mutex_cola_new);

    log_info(LOGGER_KERNEL, "## (<%d>:<0>) Se crea el proceso - Estado: NEW", pcb->PID);

    inicializar_proceso(pcb, path_proceso);
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
    
    // contexto->motivo_desalojo = ESTADO_INICIAL;
    contexto->motivo_finalizacion = INCIAL;

    return contexto;
}

// ENVIA EL PROCECSO A MEMORIA E INTENTA INICIALIZARLO
int inicializar_proceso(t_pcb* pcb, char* path_proceso) {
    enviar_proceso_memoria(socket_kernel_memoria, pcb, PROCESS_CREATE); 
    sem_wait(&sem_mensaje);
    
    pthread_mutex_lock(&mutex_mensaje);

    if (mensaje_okey) {
        mensaje_okey = false;
        pthread_mutex_unlock(&mutex_mensaje);
        log_info(LOGGER_KERNEL, "Proceso %d inicializado y movido a READY", pcb->PID);
        mover_a_ready(pcb);
        t_tcb* hilo_cero = list_get(pcb->TIDS, 0);
        mover_hilo_a_ready(hilo_cero);
        return 1;
    } else {
        pthread_mutex_unlock(&mutex_mensaje);
        return -1;
    }
    
}

void mover_a_ready(t_pcb* pcb) {
    // REMUEVE EL PROCESO DE LA COLA NEW
    pthread_mutex_lock(&mutex_cola_new);
    eliminar_pcb_lista(cola_new, pcb->PID);
    pthread_mutex_unlock(&mutex_cola_new);

    pcb->ESTADO = READY;
}

void mover_hilo_a_ready(t_tcb* hilo) {
    pthread_mutex_lock(&mutex_cola_ready);
    if(strcmp(ALGORITMO_PLANIFICACION, "CMN") == 0) {
        agregar_hilo_a_cola(hilo);
    } else {
        list_add(cola_ready, hilo);
    }
    pthread_mutex_unlock(&mutex_cola_ready);
}

void eliminar_pcb_lista(t_list* lista, uint32_t pid) {
    for(int i=0; i < list_size(lista); i++) {
        t_pcb* pcb_actual = list_get(lista, i);
        if(pcb_actual->PID == pid){
            list_remove(lista, i);
            break;
        }
    }
}

void eliminar_tcb_lista(t_list* lista, uint32_t tid) {
    for(int i=0; i < list_size(lista); i++) {
        t_tcb* tcb_actual = list_get(lista, i);
        if(tcb_actual->TID == tid){
            list_remove(lista, i);
            break;
        }
    }
}

//OJO CON ESTA FUNCION
void mover_a_exit(t_pcb* pcb) {
    pcb->ESTADO = EXIT;
    pthread_mutex_lock(&mutex_cola_exit);
    list_add(cola_exit, pcb);
    pthread_mutex_unlock(&mutex_cola_exit);
    
    log_info(LOGGER_KERNEL, "## Finaliza el proceso <%d>", pcb->PID);
}

// INTENTA INICIALIZAR EL SIGUIENTE PROCESO EN LA COLA NEW SI HAY UNO DISPONIBLE
void intentar_inicializar_proceso_de_new() {
    pthread_mutex_lock(&mutex_cola_new);
    if (!list_is_empty(cola_new)) {
        t_pcb* pcb = list_remove(cola_new, 0); // Guarda con lo que retorna list_remove

        char* path_proceso = list_get(tabla_paths, pcb->PID);

        if (path_proceso == NULL) {
            log_error(LOGGER_KERNEL, "Error: No se encontró el path para el proceso %d", pcb->PID);
            pthread_mutex_unlock(&mutex_cola_new);
            return;
        }

        // COMENTARIO IMPORTANTE QUE NO ME ACORDABA
        // LA TABLA DE PATHS ES UNA FORMA QUE NECONTRAMOS CON FELI PARA EVITAR QUE EL
        // PROCESO TENGA ASIGNADO EL PATH DEL ARCHIVO EN SU STRUCT, SI NO QUE ESTEN
        // TODOS LOS PATHS EN UNA LISTA DE TIP CHAR* DONDE CADA POSICION CORRESPONDE
        // AL PID DE CADA PROCESO, PODRIAMOS HACER QUE LA TABLA DE PATHS SEA DE
        // TIPO CHAR** O DE TIPO T_LIST

        // SOLO MUEVE EL PROCESO A READY SI PUEDE
        // NO TRATA DE EJECUTAR EL PROXIMO

        pthread_mutex_unlock(&mutex_cola_new);
        if(inicializar_proceso(pcb, path_proceso) == -1) {
            pthread_mutex_lock(&mutex_cola_new);
            list_add(cola_new, pcb);
            pthread_mutex_unlock(&mutex_cola_new);
        }
    } else {
        pthread_mutex_unlock(&mutex_cola_new);
    }

    intentar_mover_a_execute();
}

// MUEVE UN PROCESO A EXIT LIBERANDO SUS RECURSOS E INTENTA INICIALIZAR OTRO A NEW
void process_exit(t_pcb* pcb) {
    mover_a_exit(pcb);
    enviar_proceso_memoria(socket_kernel_memoria, pcb, PROCESS_EXIT);

    sem_wait(&sem_mensaje);
    if(!mensaje_okey) {
        log_error(LOGGER_KERNEL, "ERROR AL LIBERAR EL PROCESO EN MEMORIA");
        exit(EXIT_FAILURE);
    }

    eliminar_pcb_lista(tabla_procesos, pcb->PID);
    eliminar_path(pcb->PID);
    liberar_recursos_proceso(pcb);
    intentar_inicializar_proceso_de_new();
}

void liberar_recursos_proceso(t_pcb* pcb) {
    if (pcb->TIDS != NULL) {
        list_destroy_and_destroy_elements(pcb->TIDS, free);
    }
    if (pcb->MUTEXS != NULL) {
        list_destroy_and_destroy_elements(pcb->MUTEXS, free);
    }

    log_info(LOGGER_KERNEL, "Recursos del proceso %d liberados.", pcb->PID);

    // Liberar PCB
    free(pcb);
}

// MANEJO DE HILOS ==============================

// CREA UN NUEVO TCB ASOSICADO AL PROCESO Y LO CONFIGURA CON EL PSEUDOCODIGO A EJECUTAR
void thread_create(t_pcb *pcb, char* archivo_pseudocodigo, int prioridad) {

    uint32_t nuevo_tid = asignar_tid(pcb);

    // CREA EL NUEVO HILO
    t_tcb* nuevo_tcb = crear_tcb(pcb->PID, nuevo_tid, archivo_pseudocodigo, prioridad, NEW);
    list_add(pcb->TIDS, nuevo_tcb);

    // CARGA PSEUDOGODIO A EJECUTAR
    nuevo_tcb->archivo = archivo_pseudocodigo;
    
    // INFORMA A MEMORIA DE LA CREACION DEL HILO
    envio_hilo_crear(socket_kernel_memoria, nuevo_tcb, THREAD_CREATE);

    // MUEVE EL HILO A LA COLA DE READY SI PUEDE
    nuevo_tcb->ESTADO = READY;
    
    mover_hilo_a_ready(nuevo_tcb);

    log_info(LOGGER_KERNEL, "(<%d>:<%d>) Se crea el Hilo - Estado: READY", nuevo_tcb->PID_PADRE, nuevo_tcb->TID);
}

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

        mover_hilo_a_ready(tcb_actual);

        return;
    }

    log_info(LOGGER_KERNEL, "EL hilo %d esperara al hilo %d", tid_actual, tid_esperado);
    agregar_hilo_a_bloqueados(tid_esperado, tcb_actual);
}

void agregar_hilo_a_bloqueados(uint32_t tid_esperado, t_tcb* hilo_a_bloquear) {
    hilo_a_bloquear->ESTADO = BLOCK_PTHREAD_JOIN;

    t_join* hilo_bloqueado = malloc(sizeof(t_join));
    hilo_bloqueado->tid_esperado = tid_esperado;
    hilo_bloqueado->pid_hilo = hilo_a_bloquear->PID_PADRE;
    hilo_bloqueado->hilo_bloqueado = hilo_a_bloquear;

    pthread_mutex_lock(&mutex_cola_blocked);
    list_add(cola_blocked_join, hilo_bloqueado);
    pthread_mutex_unlock(&mutex_cola_blocked);
}

void tiene_algun_hilo_bloqueado_join(uint32_t tid_terminado, uint32_t pid_hilo) {
    pthread_mutex_lock(&mutex_cola_blocked);
    for(int i=0; i < list_size(cola_blocked_join); i ++) {
        t_join* act = list_get(cola_blocked_join, i);
        if(act->tid_esperado == tid_terminado && act->pid_hilo == pid_hilo) {
            list_remove(cola_blocked_join, i);
            i--;
            desbloquear_hilo_bloqueado_join(act->hilo_bloqueado);
            free(act);
        }
    }
    pthread_mutex_unlock(&mutex_cola_blocked);
}

void desbloquear_hilo_bloqueado_join(t_tcb* hilo_a_desbloquear) {
    hilo_a_desbloquear->ESTADO = READY;

    mover_hilo_a_ready(hilo_a_desbloquear);

    log_info(LOGGER_KERNEL, "Hilo %d desbloqueado del PTHREAD_JOIN", hilo_a_desbloquear->TID);
}

t_tcb* buscar_hilo_por_tid(t_pcb* pcb, uint32_t tid) {
    for (int i=0; i < list_size(pcb->TIDS); i++) {
        t_tcb* tcb = list_get(pcb->TIDS, i);
        if (tcb->TID == tid) {
            return tcb;
        }
    }
    return NULL;
}

t_pcb* obtener_pcb_padre_de_hilo(uint32_t pid) {
    for(int i=0; i < list_size(tabla_procesos); i++) {
        t_pcb* pcb_actual = list_get(tabla_procesos, i);
        if(pcb_actual->PID == pid) {
            return pcb_actual;
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
    tiene_algun_hilo_bloqueado_join(tcb->TID, tcb->PID_PADRE);
    log_info(LOGGER_KERNEL, "Hilo %d cancelado en el proceso %d", tid, pcb->PID);
    //liberar_recursos_hilo(pcb,tcb);
}

void liberar_recursos_hilo(t_pcb* pcb, t_tcb* tcb) {
    for(int i=0; i < list_size(pcb->TIDS); i++) {
        t_tcb* act = list_get(pcb->TIDS, i);
        if(act->TID == tcb->TID) {
            list_remove(pcb->TIDS, i);
            break;
        }
    }
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
    tiene_algun_hilo_bloqueado_join(tcb->TID,tcb->PID_PADRE);
    log_info(LOGGER_KERNEL, "(<%d>:<%d>) Finaliza el Hilo", tcb->PID_PADRE, tcb->TID);
    //liberar_recursos_hilo(pcb, tcb);
}

// A CHEQUEAR PARA TEMA DE MULTINIVEL
void intentar_mover_a_execute() {

    if (!cpu_libre) {
        log_info(LOGGER_KERNEL, "CPU ocupada, no se puede mover un proceso a EXECUTE");
        return;
    }

    if(strcmp(ALGORITMO_PLANIFICACION, "CMN") == 0) {
        if(!hay_algo_en_cmn() && list_size(cola_exec) == 0 && list_size(cola_blocked_io) != 0) {
            log_warning(LOGGER_KERNEL, "No hay procesos en READY ni ejecutandose");
            log_warning(LOGGER_KERNEL, "Pero hay al menos uno bloqueado por IO, se esperara a que termine");
            sem_wait(&sem_io);
        }
    } else {
        if(list_size(cola_ready) == 0 && list_size(cola_exec) == 0 && list_size(cola_blocked_io) != 0) {
            log_warning(LOGGER_KERNEL, "No hay procesos en READY ni ejecutandose");
            log_warning(LOGGER_KERNEL, "Pero hay al menos uno bloqueado por IO, se esperara a que termine");
            sem_wait(&sem_io);
        }
    }
    
    // Obtener el proximo hilo a ejecutar en base al planificador
    t_tcb* hilo_a_ejecutar = seleccionar_hilo_por_algoritmo();
    if(hilo_a_ejecutar == NULL) {
        log_info(LOGGER_KERNEL, "No hay hilos en READY para mover a EXECUTE");
        log_info(LOGGER_KERNEL, "Se termina la ejecucion del programa");
        return;
    }

    ejecutar_hilo(hilo_a_ejecutar);
}

void ejecutar_hilo(t_tcb* hilo_a_ejecutar) {
    t_pcb* pcb_padre = obtener_pcb_padre_de_hilo(hilo_a_ejecutar->PID_PADRE);

    hilo_a_ejecutar->ESTADO = EXECUTE;
    pcb_padre->ESTADO = EXECUTE;

    pthread_mutex_lock(&mutex_cola_exec);
    list_add(cola_exec, hilo_a_ejecutar);
    cpu_libre = false;
    pthread_mutex_unlock(&mutex_cola_exec);

    if(!hilo_a_ejecutar) {
        log_error(LOGGER_KERNEL, "Error al obtener el proximo hilo a ejecutar");
        return;
    }

    //enviar_proceso_cpu(socket_kernel_cpu_dispatch, pcb_padre);
    
    int resultado = enviar_hilo_a_cpu(hilo_a_ejecutar);

    if (resultado != 0) {
        log_error(LOGGER_KERNEL, "Error al enviar el hilo %d a la CPU", hilo_a_ejecutar->TID);
        hilo_a_ejecutar->ESTADO = READY;
        pcb_padre->ESTADO = READY;
        cpu_libre = true;
        mover_hilo_a_ready(hilo_a_ejecutar);
    }

    log_info(LOGGER_KERNEL, "(<%d>:<%d>) Movido a Excecute", hilo_a_ejecutar->PID_PADRE, hilo_a_ejecutar->TID);
    if(strcmp(ALGORITMO_PLANIFICACION, "CMN") == 0) {
        empezar_quantum(QUANTUM);
    }
}

// PLANIFICADOR A CORTO PLAZO ===============
t_tcb* seleccionar_hilo_por_algoritmo() {
    if (strcmp(ALGORITMO_PLANIFICACION, "FIFO") == 0) {
        return obtener_hilo_fifo();
    } else if (strcmp(ALGORITMO_PLANIFICACION, "PRIORIDADES") == 0) {
        return obtener_hilo_x_prioridad();
    } else if (strcmp(ALGORITMO_PLANIFICACION, "CMN") == 0) {
        return seleccionar_hilo_multinivel();
    }
    return NULL;
}

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
    int prioridad_max = 100000; // INICIALIZA EN UN VALOR GRANDE ASI SIEMPRE PASA 1 VEZ POR EL IF DEL FOR

    for(int i=0; i < list_size(cola_ready); i++) {
        t_tcb* hilo_actual = list_get(cola_ready, i); // OBTIENE EL HILO EN LA POSICION i DE LA COLA
        if(hilo_actual->PRIORIDAD < prioridad_max) { // VERIFICA SI EL HILO ACTUAL TIENE MAS PRIORIDAD QUE EL QUE ESTA GUARDADO
            prioridad_max = hilo_actual -> PRIORIDAD;
            hilo_a_ejecutar = hilo_actual;
        }
    }

    eliminar_tcb_lista(cola_ready, hilo_a_ejecutar->TID);

    pthread_mutex_unlock(&mutex_cola_ready);

    return hilo_a_ejecutar;
}

// COLAS MULTINIVEL (LOS HILOS DE CADA COLA SE EJECUTAN USANDO FIFO) ===============
bool hay_algo_en_cmn() {
    pthread_mutex_lock(&mutex_cola_ready);
    for(int i=0; i < list_size(colas_multinivel); i++) {
        t_cola_multinivel* cola = list_get(colas_multinivel, i);
        if(list_size(cola->cola) != 0) {    
            pthread_mutex_unlock(&mutex_cola_ready);
            return true;
        }
    }
    pthread_mutex_unlock(&mutex_cola_ready);
    return false;
}

t_tcb* seleccionar_hilo_multinivel() {
    pthread_mutex_lock(&mutex_cola_ready);
    for(int i=0; i < list_size(colas_multinivel); i++) {
        t_cola_multinivel* cola_act = list_get(colas_multinivel, i);
        if(cola_act != NULL && list_size(cola_act->cola) > 0) { //verifica que la cola no esté vacía
            t_tcb* hilo_a_ejecutar = list_remove(cola_act->cola, 0);
            pthread_mutex_unlock(&mutex_cola_ready);
            return hilo_a_ejecutar;
        }
    }
    pthread_mutex_unlock(&mutex_cola_ready);
    return NULL;
}

void agregar_hilo_a_cola(t_tcb* hilo) {
    if(list_size(colas_multinivel) - 1 < hilo->PRIORIDAD) {
        pthread_mutex_lock(&mutex_cola_multinivel);
        expandir_lista_hasta_indice(hilo->PRIORIDAD);
        pthread_mutex_unlock(&mutex_cola_multinivel);
    }

    t_cola_multinivel* cola_multinivel = buscar_cola_multinivel(hilo->PRIORIDAD);

    list_add(cola_multinivel->cola, hilo);
}

t_cola_multinivel* buscar_cola_multinivel(int prioridad) {
    if (prioridad >= list_size(colas_multinivel)) {
        log_error(LOGGER_KERNEL, "Índice fuera de rango en colas_multinivel: %d", prioridad);
        return NULL;
    }
    return list_get(colas_multinivel, prioridad);
}

void expandir_lista_hasta_indice(int indice) {
    if (!colas_multinivel) {
        log_error(LOGGER_KERNEL, "colas_multinivel no está inicializada");
        return;
    }

    int contador = list_size(colas_multinivel);

    while (indice >= contador) {
        t_cola_multinivel* cola_nueva = malloc(sizeof(t_cola_multinivel));
        if (!cola_nueva) {
            log_error(LOGGER_KERNEL, "Error al asignar espacio a la cola");
            return;
        }

        cola_nueva->nro = contador;
        cola_nueva->cola = list_create();
        if (!cola_nueva->cola) {
            log_error(LOGGER_KERNEL, "Error al crear lista dentro de cola_multinivel");
            free(cola_nueva);
            return;
        }

        list_add(colas_multinivel, cola_nueva);
        contador++;
    }

}

// OJO CON ESTA FUNCION YA QUE PUEDE BLOQUEAR TODO KERNEL
void empezar_quantum(int quantum) {
    // Espera el tiempo especificado por el quantum
    usleep(quantum * 1000); // Convertir milisegundos a microsegundos
    enviar_interrupcion_cpu(FINALIZACION_QUANTUM);

}

// ENTRADA Y SALIDA ====================
void io(t_pcb* pcb, uint32_t tid, int milisegundos) {
    t_tcb* tcb = buscar_hilo_por_tid(pcb, tid);
    if(tcb == NULL) {
        log_error(LOGGER_KERNEL, "Error: Hilo con TID %d no encontrado en el proceso %d", tid, pcb->PID);
        return;
    }

    tcb->ESTADO = BLOCK_IO;
    empezar_io(tcb->PID_PADRE, tcb->TID, milisegundos);
}

void empezar_io(uint32_t pid, uint32_t tid, int milisegundos) {
    pthread_t hilo_temporizador;

    t_io* parametros = malloc(sizeof(t_io));
    if(parametros == NULL) {
        log_error(LOGGER_KERNEL, "Error al asignar memoria");
        return;
    }
    parametros->pid_hilo = pid;
    parametros->tid = tid;
    parametros->milisegundos = milisegundos;

    agregar_hilo_a_bloqueados_io(parametros);

    if(pthread_create(&hilo_temporizador, NULL, ejecutar_temporizador_io, (void*)parametros) != 0) {
        log_error(LOGGER_KERNEL, "Error al hacer el IO");
        free(parametros);
        return;
    }

    pthread_detach(hilo_temporizador);

}

void* ejecutar_temporizador_io(void* args) {
    t_io* parametros = (t_io*)args;

    log_warning(LOGGER_KERNEL, "## (<%d>:<%d>) - Bloqueado por <IO> durante %d", parametros->pid_hilo, parametros->tid, parametros->milisegundos);
    intentar_mover_a_execute();

    usleep(30000 * 1000);//parametros->milisegundos * 1000);
    desbloquear_hilo_bloqueado_io(parametros->pid_hilo, parametros->tid);
    return NULL;
}

void agregar_hilo_a_bloqueados_io(t_io* hilo) {
    pthread_mutex_lock(&mutex_cola_blocked);
    list_add(cola_blocked_io, hilo);
    pthread_mutex_unlock(&mutex_cola_blocked);
}

void desbloquear_hilo_bloqueado_io(uint32_t pid, uint32_t tid) {
    uint32_t pid_desbloqueado = -1;
    uint32_t tid_desbloqueado = -1;

    for(int i=0; i < list_size(cola_blocked_io); i++) {
        t_io* io = list_get(cola_blocked_io, i);
        if(io->tid == tid && io->pid_hilo == pid) {
            list_remove(cola_blocked_io, i);
            pid_desbloqueado = io->pid_hilo;
            tid_desbloqueado = io->tid;
            free(io);
            break;
        }
    }

    if(pid_desbloqueado == -1 || tid_desbloqueado == -1) {
        log_error(LOGGER_KERNEL, "ERROR AL OBTENER LOS IDENTIFICADORES");
        return;
    }

    t_pcb* pcb = obtener_pcb_padre_de_hilo(pid_desbloqueado);
    t_tcb* tcb = buscar_hilo_por_tid(pcb, tid_desbloqueado);

    log_warning(LOGGER_KERNEL, "## (<%d>:<%d>) - Finalizo IO y pasa a READY", tcb->PID_PADRE, tcb->TID);

    if(strcmp(ALGORITMO_PLANIFICACION, "CMN") == 0) {
        if(!hay_algo_en_cmn() && list_size(cola_exec) == 0) {
            tcb->ESTADO = READY;
            mover_hilo_a_ready(tcb);
            sem_post(&sem_io);
        } else {
            mover_hilo_a_ready(tcb);
        }
    } else {
        if(list_size(cola_ready) == 0 && list_size(cola_exec) == 0) {
            tcb->ESTADO = READY;
            mover_hilo_a_ready(tcb);
            sem_post(&sem_io);
        } else {
            mover_hilo_a_ready(tcb);
        }
    }

}

// MANEJO DE TABLA PATHS
//void agregar_path(uint32_t pid, char* archivo) {
//    char* path_copiado = strdup(archivo);
//    list_add_in_index(tabla_paths, pid, path_copiado);
//}

void agregar_path(uint32_t pid, char* archivo) {
    char* path_copiado = strdup(archivo);
    while (list_size(tabla_paths) <= pid) {
        list_add(tabla_paths, NULL);
    }
    list_replace(tabla_paths, pid, path_copiado);
}

char* obtener_path(uint32_t pid) {
    return (char*) list_get(tabla_paths, pid);
}

void eliminar_path(uint32_t pid) {
    char* archivo = list_get(tabla_paths, pid);
    free(archivo);
    list_replace(tabla_paths, pid, NULL);
}
