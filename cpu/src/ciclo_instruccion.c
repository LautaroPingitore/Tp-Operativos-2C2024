#include "include/gestor.h"

// VARIABLES GLOBALES
t_proceso_cpu* pcb_actual = NULL;
t_tcb* hilo_actual = NULL;
bool hay_syscall = false;

pthread_mutex_t mutex_syscall;

void ejecutar_ciclo_instruccion() {

    while (1) {

        if (!hilo_actual) {
            log_error(LOGGER_CPU, "No hay Hilo actual. Terminando ciclo de instrucción.");
            break;
        }

        if(!pcb_actual) {
            log_error(LOGGER_CPU, "No hay Proceso actual. Terminando ciclo de instrucción.");
            break;
        }

        t_instruccion *instruccion = fetch(hilo_actual->PID_PADRE, hilo_actual->TID, hilo_actual->PC);
        
        if (!instruccion) {
            log_error(LOGGER_CPU, "Error al obtener la instrucción. Terminando ciclo.");
            break;
        }

        // Aquí se llama a decode, pero por simplicidad se omite en este ejemplo
        execute(instruccion, socket_cpu_dispatch_kernel, hilo_actual);
        
        pthread_mutex_lock(&mutex_syscall);
        if(hay_syscall) {
            actualizar_listas_cpu(pcb_actual, hilo_actual);
            pthread_mutex_unlock(&mutex_syscall);
            liberar_instruccion(instruccion);
            return;
        }
        pthread_mutex_unlock(&mutex_syscall);

        // Verifica si se necesita realizar un check_interrupt
        check_interrupt();
        if(hay_interrupcion) {
            return;
        }

        liberar_instruccion(instruccion);

    }
}

// RECIBE LA PROXIMA EJECUCION A REALIZAR OBTENIDA DEL MODULO MEMORIA
t_instruccion *fetch(uint32_t pid, uint32_t tid, uint32_t pc) {
    pedir_instruccion_memoria(pid, tid, pc, socket_cpu_memoria);
    t_instruccion* inst = malloc(sizeof(t_instruccion));

    sem_wait(&sem_instruccion);

    inst = instruccion_actual;
    
    if (!inst) {
        log_error(LOGGER_CPU, "Fallo al deserializar la instrucción.");
        return NULL;
    }

    log_info(LOGGER_CPU, "## (<%d> : <%d>) - FETCH - Program Counter: <%d>", pid, tid, pc);

    return inst;
}

nombre_instruccion string_a_enum(char* instruccion) {
    if (strcmp(instruccion, "SUM") == 0) {
        return SUM;
    } else if (strcmp(instruccion, "READ_MEM") == 0) {
        return READ_MEM;
    } else if (strcmp(instruccion, "WRITE_MEM") == 0) {
        return WRITE_MEM;
    } else if (strcmp(instruccion, "JNZ") == 0) {
        return JNZ;
    } else if (strcmp(instruccion, "SET") == 0) {
        return SET;
    } else if (strcmp(instruccion, "SUB") == 0) {
        return SUB;
    } else if (strcmp(instruccion, "LOG") == 0) {
        return LOG;
    } else if (strcmp(instruccion, "DUMP_MEMORY") == 0) {
        return INST_DUMP_MEMORY;
    } else if (strcmp(instruccion, "IO") == 0) {
        return INST_IO;
    } else if (strcmp(instruccion, "PROCESS_CREATE") == 0) {
        return INST_PROCESS_CREATE;
    } else if (strcmp(instruccion, "THREAD_CREATE") == 0) {
        return INST_THREAD_CREATE;
    } else if (strcmp(instruccion, "THREAD_JOIN") == 0) {
        return INST_THREAD_JOIN;
    } else if (strcmp(instruccion, "THREAD_CANCEL") == 0) {
        return INST_THREAD_CANCEL;
    } else if (strcmp(instruccion, "MUTEX_CREATE") == 0) {
        return INST_MUTEX_CREATE;
    } else if (strcmp(instruccion, "MUTEX_LOCK") == 0) {
        return INST_MUTEX_LOCK;
    } else if (strcmp(instruccion, "MUTEX_UNLOCK") == 0) {
        return INST_MUTEX_UNLOCK;
    } else if (strcmp(instruccion, "THREAD_EXIT") == 0) {
        return INST_THREAD_EXIT;
    } else if (strcmp(instruccion, "PROCESS_EXIT") == 0) {
        return INST_PROCESS_EXIT;
    }
    return ERROR_INSTRUCCION;
}

// EJECUTA LA INSTRUCCION OBTENIDA, Y TAMBIEN HACE EL DECODE EN CASO DE NECESITARLO
void execute(t_instruccion *instruccion, int socket, t_tcb* tcb) {
    nombre_instruccion op_code = string_a_enum(instruccion->nombre);
    switch (op_code) {
        case SUM:
            loguear_y_sumar_pc(instruccion);
            sum_registros(instruccion->parametro1, instruccion->parametro2);
            break;
        case READ_MEM:
            loguear_y_sumar_pc(instruccion);
            read_mem(instruccion->parametro1, instruccion->parametro2);
            break;
        case WRITE_MEM:
            loguear_y_sumar_pc(instruccion);
            write_mem(instruccion->parametro1, instruccion->parametro2);
            break;
        case JNZ:
            loguear_y_sumar_pc(instruccion);
            uint32_t nro_pc = atoi(instruccion->parametro2);
            jnz_pc(instruccion->parametro1, nro_pc);
            break;
        case SET:
            loguear_y_sumar_pc(instruccion);
            set_registro(instruccion->parametro1, instruccion->parametro2);
            break;
        case SUB:
            loguear_y_sumar_pc(instruccion);
            sub_registros(instruccion->parametro1, instruccion->parametro2);
            break;
        case LOG:
            log_registro(instruccion->parametro1);
            hilo_actual->PC++;
            break;

        // SYSCALLS QUE DEVUELVEN EL CONTROL A KERNEL
        case INST_DUMP_MEMORY:
            loguear_y_sumar_pc(instruccion);
            actualizar_contexto_memoria();

            pthread_mutex_lock(&mutex_syscall);
            hay_syscall = true;
            pthread_mutex_unlock(&mutex_syscall);

            enviar_syscall_kernel(instruccion, DUMP_MEMORY);
            break;
        case INST_IO:
            loguear_y_sumar_pc(instruccion);
            actualizar_contexto_memoria();

            pthread_mutex_lock(&mutex_syscall);
            hay_syscall = true;
            pthread_mutex_unlock(&mutex_syscall);
            
            enviar_syscall_kernel(instruccion, IO);
            break;
        case INST_PROCESS_CREATE:
            loguear_y_sumar_pc(instruccion);
            actualizar_contexto_memoria();
            
            pthread_mutex_lock(&mutex_syscall);
            hay_syscall = true;
            pthread_mutex_unlock(&mutex_syscall);
            
            enviar_syscall_kernel(instruccion, PROCESS_CREATE);
            break;
        case INST_THREAD_CREATE:
            loguear_y_sumar_pc(instruccion);
            actualizar_contexto_memoria();

            pthread_mutex_lock(&mutex_syscall);
            hay_syscall = true;
            pthread_mutex_unlock(&mutex_syscall);
            
            enviar_syscall_kernel(instruccion, THREAD_CREATE);
            break;
        case INST_THREAD_JOIN:
            loguear_y_sumar_pc(instruccion);
            actualizar_contexto_memoria();
            
            pthread_mutex_lock(&mutex_syscall);
            hay_syscall = true;
            pthread_mutex_unlock(&mutex_syscall);

            
            enviar_syscall_kernel(instruccion, THREAD_JOIN);
            break;
        case INST_THREAD_CANCEL:
            loguear_y_sumar_pc(instruccion);
            actualizar_contexto_memoria();

            pthread_mutex_lock(&mutex_syscall);
            hay_syscall = true;
            pthread_mutex_unlock(&mutex_syscall);

            enviar_syscall_kernel(instruccion, THREAD_CANCEL);
            break;
        case INST_MUTEX_CREATE:
            loguear_y_sumar_pc(instruccion);
            actualizar_contexto_memoria();
            
            pthread_mutex_lock(&mutex_syscall);
            hay_syscall = true;
            pthread_mutex_unlock(&mutex_syscall);
            
            enviar_syscall_kernel(instruccion, MUTEX_CREATE);
            break;
        case INST_MUTEX_LOCK:
            //loguear_y_sumar_pc(instruccion);
            log_warning(LOGGER_CPU, "SOLICITO %d:%d MUTEX LOCK", hilo_actual->PID_PADRE, hilo_actual->TID);
            hilo_actual->PC ++;
            actualizar_contexto_memoria();
            
            pthread_mutex_lock(&mutex_syscall);
            hay_syscall = true;
            pthread_mutex_unlock(&mutex_syscall);
            
            enviar_syscall_kernel(instruccion, MUTEX_LOCK);
            break;
        case INST_MUTEX_UNLOCK:
            loguear_y_sumar_pc(instruccion);
            actualizar_contexto_memoria();
            
            pthread_mutex_lock(&mutex_syscall);
            hay_syscall = true;
            pthread_mutex_unlock(&mutex_syscall);
            
            enviar_syscall_kernel(instruccion, MUTEX_UNLOCK);
            break;
        case INST_THREAD_EXIT:
            loguear_y_sumar_pc(instruccion);
            actualizar_contexto_memoria();
            
            pthread_mutex_lock(&mutex_syscall);
            hay_syscall = true;
            pthread_mutex_unlock(&mutex_syscall);
            
            enviar_syscall_kernel(instruccion, THREAD_EXIT);
            break;
        case INST_PROCESS_EXIT:
            loguear_y_sumar_pc(instruccion);
            actualizar_contexto_memoria();
            
            pthread_mutex_lock(&mutex_syscall);
            hay_syscall = true;
            pthread_mutex_unlock(&mutex_syscall);

            enviar_syscall_kernel(instruccion, PROCESS_EXIT);
            break;
        default:
            log_warning(LOGGER_CPU, "Instrucción desconocida.");
            break;
    }

}

// VERIFICA SI SE RECIBIO UNA INTERRUPCION POR PARTE DE KERNEL
// PODRIA RECIBIR UN PAQUETE CON EL NOMBRE DE LA INTERRUPCCION
void check_interrupt() {
    //recibir_interrupcion(socket_cpu_interrupt_kernel);
    if(hay_interrupcion) {
        log_info(LOGGER_CPU, "## Llega interrupción al puerto Interrupt");
        actualizar_listas_cpu(pcb_actual, hilo_actual);
        actualizar_contexto_memoria();
        devolver_control_al_kernel();
    }
}

// MUESTRA EN CONSOLA LA INSTRUCCION EJECUTADA Y LE SUMA 1 AL PC
void loguear_y_sumar_pc(t_instruccion *instruccion) {
    if(instruccion->parametro3 == -1) {
        log_info(LOGGER_CPU, "## TID: %d - Ejecutando: %s - Parametros: %s %s", hilo_actual->TID, instruccion->nombre, instruccion->parametro1, instruccion->parametro2);
    } else {
        log_info(LOGGER_CPU, "## TID: %d - Ejecutando: %s - Parametros: %s %s %d", hilo_actual->TID, instruccion->nombre, instruccion->parametro1, instruccion->parametro2, instruccion->parametro3);
    }
    hilo_actual->PC++;
}

void liberar_instruccion(t_instruccion *instruccion) {
    free(instruccion->nombre);
    free(instruccion->parametro1);
    free(instruccion->parametro2);
    free(instruccion);
}

void actualizar_contexto_memoria() {
    // Se puede suponer que la actualización del contexto implica actualizar los registros y el PC
    if (!hilo_actual) {
        log_error(LOGGER_CPU, "No hay Hilo actual. No se puede actualizar contexto.");
        return;
    }

    // Enviar los registros y el program counter a memoria
    // A través de la memoria se actualizaría el PCB
    enviar_contexto_memoria(hilo_actual->PID_PADRE, hilo_actual->TID, pcb_actual->REGISTROS, socket_cpu_memoria);
    log_info(LOGGER_CPU, "## TID <%d> - Actualizo Contexto Ejecucion", hilo_actual->TID);
    sem_wait(&sem_mensaje);
    if(!mensaje_okey) {
        log_error(LOGGER_CPU, "ERROR AL ACTUALIZAR EL CONTEXTO EN MEMORIA");
    }
}


t_tcb* esta_hilo_guardado(t_tcb* tcb) {
    for(int i=0; i < list_size(hilos_ejecutados); i++) {
        t_tcb* tcb_act = list_get(hilos_ejecutados, i);
        if(tcb->TID == tcb_act->TID && tcb->PID_PADRE == tcb_act->PID_PADRE) {
            return tcb_act;
        }
    }
    return NULL;
}

t_proceso_cpu* esta_proceso_guardado(t_proceso_cpu* pcb) {
    for(int i=0; i < list_size(procesos_ejecutados); i++) {
        t_proceso_cpu* pcb_act = list_get(procesos_ejecutados, i);
        if(pcb->PID == pcb_act->PID) {
            return pcb_act;
        }
    }
    return NULL;
}

void actualizar_listas_cpu(t_proceso_cpu* pcb, t_tcb* tcb) {
    for(int i=0; i < list_size(procesos_ejecutados); i++) {
        t_proceso_cpu* pcb_act = list_get(procesos_ejecutados, i);
        if(pcb->PID == pcb_act->PID) {
            list_replace(procesos_ejecutados, i, pcb);
        }
    }

    for(int j=0; j < list_size(hilos_ejecutados); j++) {
        t_tcb* tcb_act = list_get(hilos_ejecutados, j);
        if(tcb->TID == tcb_act->TID && tcb_act->PID_PADRE == tcb->PID_PADRE) {
            list_replace(hilos_ejecutados, j, tcb);
        }
    }
}