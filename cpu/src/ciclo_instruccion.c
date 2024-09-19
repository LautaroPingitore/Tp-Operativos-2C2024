#include <include/ciclo_instruccion.h>

/*
// FALTA DECLARAR BIEN TODOS LOS STRUCTS Y TODOS LOS ENUMS

void ciclo_instruccion(){
    fetch(); ===== FALTA VERIFICAR SI ESTA OK
    decode(); ====== VERIFICAR QUE ESTE BIEN LAS TRADUCCINES Y METERLO EN EL EXCECUTE
    excecute(); ======= SOLO HAY QUE DESARROLLAR CADA FUNCION EN UN ARCHVIO NUEVO 
    check_interrupt(); ======== HACER LAS 2 FUNCIONES DE ACTUALIZAR CONTEXTO Y DEVOLVER CONTROL KERNEL
}
*/

t_pcb *pcb_actual;
int fd_cpu_memoria;

void ejecutar_ciclo_instruccion(int socket)
{
    t_instruccion *instruccion = fetch(pcb_actual->pid, pcb_actual->contexto_ejecucion->registros->program_counter);
    // TODO decode: manejo de TLB y MMU
    execute(instruccion, socket);
    liberar_instruccion(instruccion);
}

// RECIBE LA PROXIMA EJECUCION A REALIZAR OBTENIDA DEL MODULO MEMORIA
t_instruccion *fetch(uint32_t pid, uint32_t pc)
{
    pedir_instruccion_memoria(pid, pc, fd_cpu_memoria);

    op_code codigo_op = recibir_operacion(fd_cpu_memoria);

    t_instruccion *instruccion;

    if (codigo_op == INSTRUCCION) {
        instruccion = deserializar_instruccion(fd_cpu_memoria);
    }
    else
    {
        log_warning(LOGGER_CPU, "Operacion desconocida. No se pudo recibir la instruccion de memoria.");
        exit(EXIT_FAILURE);
    }

    log_info(LOGGER_CPU, "PID: %d - FETCH - Program Counter: %d", pid, pc);

    return instruccion;
}

// EJECUTA LA INSTRUCCION OBTENIDA, Y TAMBIEN HACE EL DECODE EN CASO DE NECESITARLO
void execute(t_instruccion *instruccion, int socket)
{
    switch (instruccion->nombre)
    {
    case SUM:
        loguear_y_sumar_pc(instruccion);
        sum_registros(instruccion->parametro1, instruccion->parametro2);
        break;
    case READ_MEM:
        loguear_y_sumar_pc(instruccion);
        read_mem(instruccion->parametro1, instruccion->parametro2, fd_cpu_memoria); // cambiar nombre _mov_in
        break;
    case WRITE_MEM:
        loguear_y_sumar_pc(instruccion);
        write_mem(instruccion->parametro1, instruccion->parametro2, fd_cpu_memoria); // cambiar nombre _mov_out
    case JNZ:
        loguear_y_sumar_pc(instruccion);
        jnz_pc(instruccion->parametro1, instruccion->parametro2);
        break;
    case SET:
        loguear_y_sumar_pc(instruccion);
        set_registro(instruccion->parametro1, instruccion->parametro2);
        break;
    case SUB:
        loguear_y_sumar_pc(instruccion);
        sub_registros(instruccion->parametro1, instruccion->parametro2);
        break;
        break;
    case LOG:
        loguear_y_sumar_pc(instruccion);
        log_registro(instruccion->parametro1,instruccion->parametro2);
        break;
    }
}

// VERIFICA SI SE RECIBIO UNA INTERRUPCION POR PARTE DE KERNEL
// void check_interrupt() {
//     if (recibir_interrupcion(fd_cpu_interrupt)) {
//         log_info(LOGGER_CPU, "Interrupción recibida. Actualizando contexto y devolviendo control al Kernel.");
//         actualizar_contexto_memoria();
//         devolver_control_al_kernel();
//     }
// }

// MUESTRA EN CONSOLA LA INSTRUCCION EJECUTADA Y LE SUMA 1 AL PC
void loguear_y_sumar_pc(t_instruccion *instruccion)
{
    log_instruccion_ejecutada(instruccion->nombre, instruccion->parametro1, instruccion->parametro2, instruccion->parametro3);
    pcb_actual->contexto_ejecucion->registros->program_counter++;
}

// MUESTRA EN EL LOGGER LA INSTRUCCION EJECUTADA
void log_instruccion_ejecutada(nombre_instruccion nombre, char *param1, char *param2, char *param3)
{
    char *nombre_instruccion = instruccion_to_string(nombre);
    log_info(LOGGER_CPU, "PID: %d - Ejecutando: %s - Parametros: %s %s %s %s %s", pcb_actual->pid, nombre_instruccion, param1, param2, param3);
}

// LO QUE HACE ES CONVERTIR UNA INSTRUCCION RECIBIDA A FORMATO STRING PARA QUE PUEDA ESCRIBIRSE EN LA CONSOLA
char *instruccion_to_string(nombre_instruccion nombre) 
{
    switch (nombre)
    {
    case SET:
        return "SET";
    case SUM:
        return "SUM";
    case SUB:
        return "SUB";
    case JNZ:
        return "JNZ";
    case READ_MEM:
        return "READ_MEM";
    case WRITE_MEM:
        return "WRITE_MEM";
    case LOG:
        return "LOG";
    default:
        return "DESCONOCIDA";
    }
}

void pedir_instruccion_memoria(uint32_t pid, uint32_t pc, int socket)
{
    t_paquete *paquete = crear_paquete_con_codigo_de_operacion(PEDIDO_INSTRUCCION);
    paquete->buffer->size += sizeof(uint32_t) * 2;
    paquete->buffer->stream = malloc(paquete->buffer->size);
    memcpy(paquete->buffer->stream, &(pid), sizeof(uint32_t));
    memcpy(paquete->buffer->stream + sizeof(uint32_t), &(pc), sizeof(int));
    enviar_paquete(paquete, socket);
    eliminar_paquete(paquete);
}

t_instruccion *deserializar_instruccion(int socket)
{
    t_paquete *paquete = recibir_paquete(socket);
    t_instruccion *instruccion = malloc(sizeof(t_instruccion));

    void *stream = paquete->buffer->stream;
    int desplazamiento = 0;

    memcpy(&(instruccion->nombre), stream + desplazamiento, sizeof(nombre_instruccion));
    desplazamiento += sizeof(nombre_instruccion);

    uint32_t tamanio_parametro1;
    memcpy(&(tamanio_parametro1), stream + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);

    uint32_t tamanio_parametro2;
    memcpy(&(tamanio_parametro2), stream + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);

    uint32_t tamanio_parametro3;
    memcpy(&(tamanio_parametro3), stream + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);

    instruccion->parametro1 = malloc(tamanio_parametro1);
    memcpy(instruccion->parametro1, stream + desplazamiento, tamanio_parametro1);
    desplazamiento += tamanio_parametro1;

    instruccion->parametro2 = malloc(tamanio_parametro2);
    memcpy(instruccion->parametro2, stream + desplazamiento, tamanio_parametro2);
    desplazamiento += tamanio_parametro2;

    instruccion->parametro3 = malloc(tamanio_parametro3);
    memcpy(instruccion->parametro3, stream + desplazamiento, tamanio_parametro3);
    desplazamiento += tamanio_parametro3;

    eliminar_paquete(paquete);

    return instruccion;
}

void liberar_instruccion(t_instruccion *instruccion) {
    free(instruccion->parametro1);
    free(instruccion->parametro2);
    free(instruccion->parametro3);
    free(instruccion);
}