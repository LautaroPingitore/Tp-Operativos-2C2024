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

=== LOGS OBLIGATORIOS (MARCAR CON X AQUELLOS QUE YA ESTEN) ===
X Syscall recibida: ## (<PID>:<TID>) - Solicitó syscall: <NOMBRE_SYSCALL>
X Creación de Proceso: ## (<PID>:0) Se crea el proceso - Estado: NEW 
X Creación de Hilo: ## (<PID>:<TID>) Se crea el Hilo - Estado: READY
- Motivo de Bloqueo: ## (<PID>:<TID>) - Bloqueado por: <PTHREAD_JOIN / MUTEX / IO>
X Fin de IO: ## (<PID>:<TID>) finalizó IO y pasa a READY
X Fin de Quantum: ## (<PID>:<TID>) - Desalojado por fin de Quantum
X Fin de Proceso: ## Finaliza el proceso <PID>
X Fin de Hilo: ## (<PID>:<TID>) Finaliza el hilo

*/


//PLANIFICADOR LARGO PLAZO ==============================
    
// COLAS QUE EN LAS CUALES SE GUARDARAN LOS PROCESOS
t_list* cola_new;
t_list* cola_ready;
t_list* cola_exec;
t_list* cola_blocked;
t_list* cola_exit;
t_list* cola_nivel_1;
t_list* cola_nivel_2;
t_list* cola_nivel_3;

t_list* tabla_paths;
t_list* tabla_procesos;
// uint32_t pid_actual = 0;
// uint32_t tid_actual = 0;

// SEMAFOFOS QUE PROTEGEN EL ACCESO A CADA COLA
pthread_mutex_t mutex_cola_new;
pthread_mutex_t mutex_cola_ready;
pthread_mutex_t mutex_cola_exit;
pthread_mutex_t mutex_cola_blocked;
pthread_mutex_t mutex_pid;
pthread_mutex_t mutex_tid;
pthread_mutex_t mutex_estado;
pthread_cond_t cond_estado = PTHREAD_COND_INITIALIZER;

// VARIABLES DE CONTROL
uint32_t pid = 0;
uint32_t contador_tid = 0;
bool cpu_libre = true;

void inicializar_kernel() {
    cola_new = list_create();
    cola_ready = list_create();
    cola_exit = list_create();

    cola_nivel_1 = list_create();
    cola_nivel_2 = list_create();
    cola_nivel_3 = list_create();

    tabla_paths = list_create();
    tabla_procesos = list_create();

    pthread_mutex_init(&mutex_cola_new, NULL);
    pthread_mutex_init(&mutex_cola_ready, NULL);
    pthread_mutex_init(&mutex_cola_exit, NULL);
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

    // uint32_t nuevo_tid = contador_tid;
    // contador_tid++;
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

    // EL HILO PRINCIPAL SERIA MAS O MENOS LO MISMO QUE EL PROCESO EN SI
    t_tcb* hilo_principal = crear_tcb(pid, asignar_tid(pcb), archivo, 0, NEW);
    list_add(pcb->TIDS, hilo_principal);

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
    archivo_pseudocodigo* archivo = leer_archivo_pseudocodigo(path_proceso);
    t_pcb* pcb = crear_pcb(asignar_pid(), tamanio_proceso, inicializar_contexto(), NEW, path_proceso);
    obtener_recursos_del_proceso(archivo, pcb);

    // EN LA LISTA DE NEW, READY, ETC TENDRIAN QUE SER HILOS, NO PROCESOS
    pthread_mutex_lock(&mutex_cola_new);
    list_add(cola_new, pcb);
    pthread_mutex_unlock(&mutex_cola_new);

    log_info(LOGGER_KERNEL, "## (<%d>:0) Se crea el proceso - Estado: NEW", pcb->PID);

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
void inicializar_proceso(t_pcb* pcb, char* path_proceso) {
    enviar_proceso_memoria(socket_kernel_memoria, pcb, PROCESS_CREATE);
    int resultado = respuesta_memoria_creacion(socket_kernel_memoria);

    if (resultado == 1) {
        log_info(LOGGER_KERNEL, "Proceso %d inicializado y movido a READY", pcb->PID);
        mover_a_ready(pcb);
    } else {
        log_warning(LOGGER_KERNEL, "No hay espacio en memoria, se mantiene en new");
    }
}

void mover_a_ready(t_pcb* pcb) {
    // REMUEVE EL PROCESO DE LA COLA NEW
    pthread_mutex_lock(&mutex_cola_new);
    // LO ELIMINA CUANDO SU PID COINCIDE CON EL DEL PROCESO QUE SE ESTA MOVIENDO
    eliminar_pcb_lista(cola_new, pcb->PID);
    pthread_mutex_unlock(&mutex_cola_new);

    pcb->ESTADO = READY;
    // MANDA AL HILO 0 A LA COLA DE READY
    t_tcb* hilo_cero = list_get(pcb->TIDS, 0);
    hilo_cero->ESTADO = READY;
    pthread_mutex_lock(&mutex_cola_ready);
    list_add(cola_ready, hilo_cero);
    pthread_mutex_unlock(&mutex_cola_ready);
    
    log_info(LOGGER_KERNEL, "Hilo %d movido a READY ", hilo_cero->TID);
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
    // NO HACE FALTA PORQUE LOS HILOS ESTAN EL COLA READY, NO LOS PROCESOS
    // REMUEVE EL PROCESO DE LA COLA READY
    // pthread_mutex_lock(&mutex_cola_ready);
    // eliminar_pcb_lista(cola_ready, pcb->PID);
    // pthread_mutex_unlock(&mutex_cola_ready);

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

        char* path_proceso= obtener_path(pcb->PID);

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
        inicializar_proceso(pcb, path_proceso);
    }
    pthread_mutex_unlock(&mutex_cola_new);

    intentar_mover_a_execute();
}

// MUEVE UN PROCESO A EXIT LIBERANDO SUS RECURSOS E INTENTA INICIALIZAR OTRO A NEW
void process_exit(t_pcb* pcb) {
    mover_a_exit(pcb);
    liberar_recursos_proceso(pcb);
    enviar_proceso_memoria(socket_kernel_memoria, pcb, PROCESS_EXIT);
    eliminar_path(pcb->PID);
    eliminar_pcb_lista(tabla_procesos, pcb->PID);
    intentar_inicializar_proceso_de_new();
}

void liberar_recursos_proceso(t_pcb* pcb) {
    if (pcb->TIDS != NULL) {
        list_destroy_and_destroy_elements(pcb->TIDS, free);
    }
    if (pcb->MUTEXS != NULL) {
        list_destroy_and_destroy_elements(pcb->MUTEXS, free);
    }

    // Liberar PCB
    free(pcb);

    log_info(LOGGER_KERNEL, "Recursos del proceso %d liberados.", pcb->PID);
}

// MANEJO DE HILOS ==============================

// CREA UN NUEVO TCB ASOSICADO AL PROCESO Y LO CONFIGURA CON EL PSEUDOCODIGO A EJECUTAR
void thread_create(t_pcb *pcb, char* archivo_pseudocodigo, int prioridad) {

    uint32_t nuevo_tid = asignar_tid(pcb);

    // CREA EL NUEVO HILO
    t_tcb* nuevo_tcb = crear_tcb(pcb->PID, nuevo_tid, archivo_pseudocodigo, prioridad, NEW);
    list_add(pcb->TIDS, &nuevo_tid);

    // CARGA PSEUDOGODIO A EJECUTAR
    nuevo_tcb->archivo = archivo_pseudocodigo;
    
    // INFORMA A MEMORIA DE LA CREACION DEL HILO
    envio_hilo_crear(socket_kernel_memoria, nuevo_tcb, THREAD_CREATE);

    // MUEVE EL HILO A LA COLA DE READY SI PUEDE
    nuevo_tcb->ESTADO = READY;
    pthread_mutex_lock(&mutex_cola_ready);
    list_add(cola_ready, nuevo_tcb);
    pthread_mutex_unlock(&mutex_cola_ready);

    log_info(LOGGER_KERNEL, "(<%d>:<%d>) Se crea el Hilo - Estado: READY", nuevo_tcb->PID_PADRE, nuevo_tcb->TID);
}

// CUIDADO
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
        return;
    }

    log_info(LOGGER_KERNEL, "EL hilo %d esperara al hilo %d", tid_actual, tid_esperado);

    esperar_a_que_termine(tcb_esperado, tcb_actual);
}

void esperar_a_que_termine(t_tcb* esperado,  t_tcb* actual) {
    pthread_mutex_lock(&mutex_estado);

    actual->ESTADO = BLOCK_PTHREAD_JOIN;
    
    pthread_mutex_lock(&mutex_cola_blocked);
    list_add(cola_blocked, actual);
    pthread_mutex_unlock(&mutex_cola_blocked);

    while (esperado->ESTADO != EXIT) {
        pthread_cond_wait(&cond_estado, &mutex_estado);
    }

    desbloquear_hilo_actual(actual);

    pthread_mutex_unlock(&mutex_estado);
}

void desbloquear_hilo_actual(t_tcb* actual) {

    pthread_mutex_lock(&mutex_estado);

    // Cambiar el estado del hilo a READY
    actual->ESTADO = READY;
    pthread_mutex_lock(&mutex_cola_blocked);
    eliminar_tcb_lista(cola_blocked, actual->TID);
    pthread_mutex_unlock(&mutex_cola_blocked);

    pthread_mutex_lock(&mutex_cola_ready);
    list_add(cola_ready, actual);
    pthread_mutex_unlock(&mutex_cola_ready);

    log_info(LOGGER_KERNEL, "Hilo %d desbloqueado.", actual->TID);

    // Enviar señal para despertar a todos los hilos bloqueados en esta condición
    // FUNCION RARA
    pthread_cond_broadcast(&cond_estado);

    pthread_mutex_unlock(&mutex_estado);
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
    liberar_recursos_hilo(tcb);
    log_info(LOGGER_KERNEL, "Hilo %d cancelado en el proceso %d", tid, pcb->PID);

    if(list_size(pcb->TIDS) == 0) {
        process_exit(pcb);
    }
}

void liberar_recursos_hilo(t_tcb* tcb) {
    // REMUEVE EL HILO EJECUTANDOSE
    list_remove(cola_exec, 0);

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
    log_info(LOGGER_KERNEL, "(<%d>:<%d>) Finaliza el Hilo", tcb->PID_PADRE, tcb->TID);

    if(list_size(pcb->TIDS) == 0) {
        process_exit(pcb);
    }
}

// A CHEQUEAR
void intentar_mover_a_execute() {
    // Verificar si hay algun proceso en READY y si la CPU esta libre
    pthread_mutex_lock(&mutex_cola_ready);
    if (list_is_empty(cola_ready)) {
        log_info(LOGGER_KERNEL, "No hay procesos en READY para mover a EXECUTE");
        pthread_mutex_unlock(&mutex_cola_ready);
        return;
    }

    if (!cpu_libre) {
        log_info(LOGGER_KERNEL, "CPU ocupada, no se puede mover un proceso a EXECUTE");
        pthread_mutex_unlock(&mutex_cola_ready);
        return;
    }

    // Obtener el proximo hilo a ejecutar en base al planificador
    t_tcb* hilo_a_ejecutar = seleccionar_hilo_por_algoritmo();
    eliminar_tcb_lista(cola_ready, hilo_a_ejecutar->TID);
    pthread_mutex_unlock(&mutex_cola_ready);

    t_pcb* pcb_padre = obtener_pcb_padre_de_hilo(hilo_a_ejecutar->PID_PADRE);
    hilo_a_ejecutar->ESTADO = EXECUTE;
    pcb_padre->ESTADO = EXECUTE;
    list_add(cola_exec, hilo_a_ejecutar);
    cpu_libre = false;

    if(!hilo_a_ejecutar) {
        log_error(LOGGER_KERNEL, "Error al obtener el proximo hilo a ejecutar");
        return;
    }

    log_info(LOGGER_KERNEL, "(<%d>:<%d>) Movido a Excecute", hilo_a_ejecutar->PID_PADRE, hilo_a_ejecutar->TID);

    int resultado = enviar_hilo_a_cpu(hilo_a_ejecutar);

    if (resultado != 0) {
        log_error(LOGGER_KERNEL, "Error al enviar el hilo %d a la CPU", hilo_a_ejecutar->TID);
        hilo_a_ejecutar->ESTADO = READY;
        pcb_padre->ESTADO = READY;
        cpu_libre = true;
        pthread_mutex_lock(&mutex_cola_ready);
        list_add(cola_ready, hilo_a_ejecutar);
        pthread_mutex_lock(&mutex_cola_ready);
    } else {
        log_info(LOGGER_KERNEL, "El hilo %d ha sido enviado a la CPU", hilo_a_ejecutar->TID);
    }
}

// PLANIFICADOR A CORTO PLAZO ===============
t_tcb* seleccionar_hilo_por_algoritmo() {
    if (strcmp(ALGORITMO_PLANIFICACION, "FIFO") == 0) {
        return obtener_hilo_fifo();
    } else if (strcmp(ALGORITMO_PLANIFICACION, "PRIORIDADES") == 0) {
        return obtener_hilo_x_prioridad();
    } else if (strcmp(ALGORITMO_PLANIFICACION, "COLAS_MULTINIVEL") == 0) {
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

t_tcb* seleccionar_hilo_multinivel() {
    pthread_mutex_lock(&mutex_cola_ready);
    t_tcb* siguiente_hilo = NULL;

    if (!list_is_empty(cola_nivel_1)) {
        siguiente_hilo = list_remove(cola_nivel_1, 0);
    } else if (!list_is_empty(cola_nivel_2)) {
        siguiente_hilo = list_remove(cola_nivel_2, 0);
    } else if (!list_is_empty(cola_nivel_3)) {
        siguiente_hilo = list_remove(cola_nivel_3, 0);
    }
    pthread_mutex_unlock(&mutex_cola_ready);

    // Si no hay hilos en ninguna cola, verificar tabla de procesos
    if (siguiente_hilo == NULL) {
        pthread_mutex_lock(&mutex_cola_ready);
        if (list_is_empty(tabla_procesos)) {
            log_info(LOGGER_KERNEL, "Todos los procesos han finalizado. Deteniendo sistema.");
            exit(EXIT_SUCCESS);
        }
        pthread_mutex_unlock(&mutex_cola_ready);
    }
    return siguiente_hilo;
}

/*
void empezar_quantum(int quantum) {
    // LOGICA DE CONTEO DE QUANTUM

    if(tiempo_actual >= quantum * 1000) {
        enviar_interrupcion_cpu(FINALIZACION_QUANTUM, quantum);
    }
}
*/


/*
void ejecutar_hilo_rr(t_tcb* hilo, int quantum) {
    log_info(LOGGER_KERNEL, "(<%d>:<%d>) Enviando hilo a CPU para ejecución con quantum %d", 
             hilo->PID_PADRE, hilo->TID, quantum);

    hilo->ESTADO = EXECUTE;
    enviar_hilo_a_cpu(hilo, quantum);  // Enviar hilo a la CPU para ejecución

    // Esperar retorno desde CPU
    t_retorno_cpu* retorno = recibir_mensaje_cpu();  // Función ficticia para recibir mensajes desde CPU

    if (retorno == NULL) {
        log_error(LOGGER_KERNEL, "Error al recibir respuesta de la CPU. Reiniciando planificación.");
        hilo->ESTADO = READY;
        pthread_mutex_lock(&mutex_cola_ready);
        list_add(cola_ready, hilo);
        pthread_mutex_unlock(&mutex_cola_ready);
        return;
    }

    manejar_retorno_cpu(hilo, retorno);  // Manejar el motivo de retorno
    free(retorno);
}

*/

// void ejecutar_hilo_rr(t_tcb* hilo, int quantum) {
//     log_info(LOGGER_KERNEL, "(<%d>:<%d>) Ejecutando hilo con quantum %d", hilo->PID_PADRE, hilo->TID, quantum);

//     struct timespec inicio, actual;
//     clock_gettime(CLOCK_MONOTONIC, &inicio);

//     int tiempo_transcurrido = 0;
//     while (tiempo_transcurrido < quantum) {
//         ejecutar_hilo(hilo);  // Llamada a función ficticia para ejecutar una instrucción
//         clock_gettime(CLOCK_MONOTONIC, &actual);
//         tiempo_transcurrido = (actual.tv_sec - inicio.tv_sec) * 1000 +
//                               (actual.tv_nsec - inicio.tv_nsec) / 1000000;

//         if (check_interrupt()) {
//             log_info(LOGGER_KERNEL, "(<%d>:<%d>) Interrupción recibida. Devolviendo control al Kernel.", hilo->PID_PADRE, hilo->TID);
//             hilo->ESTADO = READY;
//             pthread_mutex_lock(&mutex_cola_ready);
//             list_add(cola_ready, hilo);
//             pthread_mutex_unlock(&mutex_cola_ready);
//             return;
//         }
//     }
//     // Si termina quantum sin salir, mover a READY
//     log_info(LOGGER_KERNEL, "(<%d>:<%d>) Finalizó quantum. Desalojado por fin de Quantum.", hilo->PID_PADRE, hilo->TID);
//     hilo->ESTADO = READY;
//     pthread_mutex_lock(&mutex_cola_ready);
//     list_add(cola_ready, hilo);
//     pthread_mutex_unlock(&mutex_cola_ready);

//     cpu_libre = true;
// }






// OJO PORQUE ACA LA EJECUCION LA HACE KERNEL Y NO CPU
// void ejecutar_hilo_rr(t_tcb* hilo, t_list* cola, int quantum) {
//     pthread_mutex_lock(&mutex_estado);
//     hilo->ESTADO = EXECUTE;
//     pthread_mutex_unlock(&mutex_estado);

//     log_info(LOGGER_KERNEL, "Ejecutando hilo TID %d del proceso por %d ms %d", hilo->TID, hilo->PID_PADRE, quantum);

//     struct timespec inicio, actual;
//     clock_gettime(CLOCK_MONOTONIC, &inicio);

//     int tiempo_transcurrido = 0;
//     while (tiempo_transcurrido < quantum) {
//         ejecutar_hilo(hilo);

//         clock_gettime(CLOCK_MONOTONIC, &actual);
//         tiempo_transcurrido = (actual.tv_sec - inicio.tv_sec) * 1000 + 
//                                 (actual.tv_nsec - inicio.tv_nsec) / 1000000;
//         usleep(1000);
//     }

//     log_info(LOGGER_KERNEL, "Hilo TID %d ejecutado durante %d ms", hilo->TID, quantum);

//     pthread_mutex_lock(&mutex_estado);
//     if(hilo->ESTADO != EXIT) {
//         hilo->ESTADO = READY;
//         pthread_mutex_unlock(&mutex_estado);

//         pthread_mutex_lock(&mutex_cola_ready); 
//         list_add(cola, hilo);
//         pthread_mutex_unlock(&mutex_cola_ready);

//         log_info(LOGGER_KERNEL, "Hilo TID %d movido a READY después del quantum", hilo->TID);
//     } else {
//         pthread_mutex_unlock(&mutex_estado);

//         log_info(LOGGER_KERNEL, "Hilo TID %d finalizado", hilo->TID);
//         liberar_recursos_hilo(hilo);
//     }

//     pthread_mutex_lock(&mutex_estado);
//     cpu_libre = true;
//     pthread_mutex_unlock(&mutex_estado);
// }

// ENTRADA Y SALIDA ====================

void io(t_pcb* pcb, uint32_t tid, int milisegundos) {
    t_tcb* tcb = buscar_hilo_por_tid(pcb, tid);
    if(tcb == NULL) {
        log_error(LOGGER_KERNEL, "Error: Hilo con TID %d no encontrado en el proceso %d", tid, pcb->PID);
        return;
    }

    tcb->ESTADO = BLOCK_IO;
    log_info(LOGGER_KERNEL, "Hilo %d en proceso %d ha iniciado IO por %d ms", tid, pcb->PID, milisegundos);

    intentar_mover_a_execute();

    usleep(milisegundos * 1000);

    tcb->ESTADO = READY;
    pthread_mutex_lock(&mutex_cola_ready);
    list_add(cola_ready, tcb);
    pthread_mutex_unlock(&mutex_cola_ready);

    log_info(LOGGER_KERNEL, "(<%d>:<%d>) Finalizó IO y pasa a READY", tcb->PID_PADRE, tcb->TID);
}

// MANEJO DE TABLA PATHS
void agregar_path(uint32_t pid, char* archivo) {
    char* path_copiado = strdup(archivo);
    list_add_in_index(tabla_paths, pid, path_copiado);
}

char* obtener_path(uint32_t pid) {
    return (char*) list_get(tabla_paths, pid);
}

void eliminar_path(uint32_t pid) {
    char* path = list_remove(tabla_paths, pid);
    if (path) free(path);
}
