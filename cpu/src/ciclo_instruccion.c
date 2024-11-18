#include "include/gestor.h"

// VARIABLES GLOBALES
t_pcb *pcb_actual = NULL;
int fd_cpu_memoria = -1;

void ejecutar_ciclo_instruccion(int socket_cliente) {
    while (true) {
        // Asegúrate de que pcb_actual esté inicializado antes de comenzar el ciclo
        if (!pcb_actual) {
            log_error(LOGGER_CPU, "No hay PCB actual. Terminando ciclo de instrucción.");
            break;
        }

        t_instruccion *instruccion = fetch(pcb_actual->pid, pcb_actual->contexto_ejecucion->registros->program_counter);
        
        if (!instruccion) {
            log_error(LOGGER_CPU, "Error al obtener la instrucción. Terminando ciclo.");
            break;
        }

        // Aquí se llama a decode, pero por simplicidad se omite en este ejemplo
        execute(instruccion, socket_cliente);
        
        // Verifica si se necesita realizar un check_interrupt
        check_interrupt();

        liberar_instruccion(instruccion);
    }
}

// RECIBE LA PROXIMA EJECUCION A REALIZAR OBTENIDA DEL MODULO MEMORIA
t_instruccion *fetch(uint32_t pid, uint32_t pc)
{
    pedir_instruccion_memoria(pid, pc, fd_cpu_memoria);

    op_code codigo_op = recibir_operacion(fd_cpu_memoria);
    if (codigo_op != INSTRUCCION) {
        log_warning(LOGGER_CPU, "Operación desconocida al obtener la instrucción de memoria.");
        return NULL;
    }

    t_instruccion *instruccion = deserializar_instruccion(fd_cpu_memoria);
    if (!instruccion) {
        log_error(LOGGER_CPU, "Fallo al deserializar la instrucción.");
        return NULL;
    }

    log_info(LOGGER_CPU, "PID: %d - FETCH - Program Counter: %d", pid, pc);

    return instruccion;
}

// EJECUTA LA INSTRUCCION OBTENIDA, Y TAMBIEN HACE EL DECODE EN CASO DE NECESITARLO
void execute(t_instruccion *instruccion, int socket)
{
    switch (instruccion->nombre) {
        case "SUM":
            loguear_y_sumar_pc(instruccion);
            sum_registros(instruccion->parametro1, instruccion->parametro2);
            break;
        case "READ_MEM":
            loguear_y_sumar_pc(instruccion);
            read_mem(instruccion->parametro1, instruccion->parametro2, socket); // cambiar nombre _mov_in
            break;
        case "WRITE_MEM":
            loguear_y_sumar_pc(instruccion);
            write_mem(instruccion->parametro1, instruccion->parametro2, socket); // cambiar nombre _mov_out
            break;
        case "JNZ":
            loguear_y_sumar_pc(instruccion);
            jnz_pc(instruccion->parametro1, instruccion->parametro2);
            break;
        case "SET":
            loguear_y_sumar_pc(instruccion);
            set_registro(instruccion->parametro1, instruccion->parametro2);
            break;
        case "SUB":
            loguear_y_sumar_pc(instruccion);
            sub_registros(instruccion->parametro1, instruccion->parametro2);
            break;
        case "LOG":
            loguear_y_sumar_pc(instruccion);
            break;
        default:
            log_warning(LOGGER_CPU, "Instrucción desconocida.");
            break;
    }

}

// VERIFICA SI SE RECIBIO UNA INTERRUPCION POR PARTE DE KERNEL
void check_interrupt() {
    if (recibir_interrupcion(fd_cpu_memoria)) {
        log_info(LOGGER_CPU, "Interrupción recibida. Actualizando contexto y devolviendo control al Kernel.");
        actualizar_contexto_memoria();
        devolver_control_al_kernel();
    }
}

// MUESTRA EN CONSOLA LA INSTRUCCION EJECUTADA Y LE SUMA 1 AL PC
void loguear_y_sumar_pc(t_instruccion *instruccion)
{
    log_info(LOGGER_CPU, "PID: %d - Ejecutando: %s - Parametros: %s %s %d", pcb_actual->pid, instruccion->nombre, intruccion->parametro1, instruccion->parametro2, instruccion->parametro3);
    pcb_actual->contexto_ejecucion->registros->program_counter++;
}

void pedir_instruccion_memoria(uint32_t pid, uint32_t pc, int socket)
{
    t_paquete *paquete = crear_paquete_con_codigo_de_operacion(PEDIDO_INSTRUCCION);
    agregar_a_paquete(paquete, &pid, sizeof(uint32_t));
    agregar_a_paquete(paquete, &pc, sizeof(uint32_t));

    enviar_paquete(paquete, socket);
    eliminar_paquete(paquete);
}

t_instruccion *deserializar_instruccion(int socket)
{
    t_paquete* paquete = recibir_paquete_entero(socket);
    if (!paquete) {
        log_error(LOGGER_CPU, "Fallo al recibir paquete.");
        return NULL;
    }

    t_instruccion* instruccion = malloc(sizeof(t_instruccion));
    void *stream = paquete->buffer->stream;
    int offset = 0;

    memcpy(&(instruccion->nombre), stream + offset, sizeof(nombre_instruccion));
    offset += sizeof(nombre_instruccion);

    uint32_t tamanio_parametro1;
    memcpy(&(tamanio_parametro1), stream + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    uint32_t tamanio_parametro2;
    memcpy(&(tamanio_parametro2), stream + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    uint32_t tamanio_parametro3;
    memcpy(&(tamanio_parametro3), stream + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    instruccion->parametro1 = malloc(tamanio_parametro1);
    memcpy(instruccion->parametro1, stream + offset, tamanio_parametro1);
    offset += tamanio_parametro1;

    instruccion->parametro2 = malloc(tamanio_parametro2);
    memcpy(instruccion->parametro2, stream + offset, tamanio_parametro2);
    offset += tamanio_parametro2;

    instruccion->parametro3 = malloc(tamanio_parametro3);
    memcpy(instruccion->parametro3, stream + offset, tamanio_parametro3);
    offset += tamanio_parametro3;

    eliminar_paquete(paquete);

    return instruccion;
}

void liberar_instruccion(t_instruccion *instruccion) {
    free(instruccion->parametro1);
    free(instruccion->parametro2);
    free(instruccion->parametro3);
    free(instruccion);
}

bool recibir_interrupcion(int socket_cliente) {
    int interrupcion = 0;

    if (recv(socket_cliente, &interrupcion, sizeof(int), 0) <= 0) {
        log_error(LOGGER_CPU, "Error al recibir interrupción del Kernel.");
        return false;
    }

    return interrupcion == 1;
}

void actualizar_contexto_memoria() {
    // Se puede suponer que la actualización del contexto implica actualizar los registros y el PC
    log_info(LOGGER_CPU, "Actualizando el contexto de la CPU en memoria...");
    if (!pcb_actual) {
        log_error(LOGGER_CPU, "No hay PCB actual. No se puede actualizar contexto.");
        return;
    }
    // Enviar los registros y el program counter a memoria
    // A través de la memoria se actualizaría el PCB
    enviar_contexto_memoria(pcb_actual->pid, pcb_actual->contexto_ejecucion->registros, pcb_actual->contexto_ejecucion->registros->program_counter, fd_cpu_memoria);

    log_info(LOGGER_CPU, "Contexto de la CPU actualizado en memoria.");
}

void enviar_contexto_memoria(uint32_t pid, t_registros* registros, uint32_t program_counter, int socket_memoria) {
    t_paquete *paquete = crear_paquete_con_codigo_de_operacion(ACTUALIZAR_CONTEXTO);

    agregar_a_paquete(paquete, &pid, sizeof(uint32_t));
    agregar_a_paquete(paquete, registros, sizeof(t_registros));
    agregar_a_paquete(paquete, &program_counter, sizeof(uint32_t));

    enviar_paquete(paquete, socket_memoria);
    eliminar_paquete(paquete);
}

void devolver_control_al_kernel() {
    log_info(LOGGER_CPU, "Devolviendo control al Kernel...");

    // Crear un paquete para notificar al Kernel
    t_paquete *paquete = crear_paquete_con_codigo_de_operacion(DEVOLVER_CONTROL_KERNEL);

    // Enviar el paquete indicando que el control se devuelve al Kernel
    enviar_paquete(paquete, socket_cpu_interrupt_kernel);  // Asegúrate de tener el socket correspondiente para el Kernel (socket_cpu_interrupt_kernel)

    eliminar_paquete(paquete);

    log_info(LOGGER_CPU, "Control devuelto al Kernel.");
}