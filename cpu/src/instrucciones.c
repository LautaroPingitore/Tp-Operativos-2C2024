#include "include/gestor.h"

#define SEGMENTATION_FAULT ((uint32_t)-1)

// Validar si una cadena es numérica
bool es_numerico(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        if (!isdigit(str[i])) {
            return false;
        }
    }
    return true;
}

// Obtener puntero al registro basado en su nombre
uint32_t* obtener_registro(char* registro){
    t_registros* regs = pcb_actual->REGISTROS;

    if (strcmp(registro, "AX") == 0) return &regs->AX;
    if (strcmp(registro, "BX") == 0) return &regs->BX;
    if (strcmp(registro, "CX") == 0) return &regs->CX;
    if (strcmp(registro, "DX") == 0) return &regs->DX;
    if (strcmp(registro, "EX") == 0) return &regs->EX;
    if (strcmp(registro, "GX") == 0) return &regs->GX;
    if (strcmp(registro, "HX") == 0) return &regs->HX;

    return NULL;
}

// Asigna al registro el valor pasado como parametro.
void set_registro(char* registro, char *valor) {
    uint32_t *reg = obtener_registro(registro);

    if (reg == NULL) {
        log_warning(LOGGER_CPU, "Error: Registro invalido en set_registros.");
        return;
    }
    if(!es_numerico(valor)) {
        log_warning(LOGGER_CPU, "Error: Valor no numérico en set_registro.");
        return;
    }

    *reg = atoi(valor);
}

//Lee el valor de memoria correspondiente a
//la Direccion Logica que se encuentra en el Registro Direccion
//y lo almacena en el Registro Datos.
void read_mem(char* reg_datos, char* reg_direccion) {
    uint32_t *registro_datos = obtener_registro(reg_datos);
    uint32_t *registro_direccion = obtener_registro(reg_direccion);

    if (!registro_datos || !registro_direccion) {
        log_warning(LOGGER_CPU, "Error: Registro inválido en read_mem.");
        return;
    }

    uint32_t direccion_fisica = traducir_direccion(*registro_direccion, pcb_actual->PID);
    if (direccion_fisica == SEGMENTATION_FAULT) {
        enviar_interrupcion_segfault(pcb_actual->PID, socket_cpu_memoria);
        return;
    }

    enviar_solicitud_valor_memoria(socket_cpu_memoria, direccion_fisica);
    sem_wait(&sem_valor_memoria); // Bloquea hasta que se reciba el valor

    *registro_datos = valor_memoria;

    log_info(LOGGER_CPU, "## TID: <%d> - Accion: <LEER> - Direccion Fisica: <%d>", hilo_actual->TID, direccion_fisica);
}

//Lee el valor del Registro Datos y lo escribe en la 
//direccion fisica de memoria obtenida a partir de la 
//Direccion Logica almacenada en el Registro Direccion.
void write_mem(char* reg_direccion, char* reg_datos) {
    uint32_t *reg_dire = obtener_registro(reg_direccion);
    uint32_t *reg_dat = obtener_registro(reg_datos);

    if (!reg_dire || !reg_dat) {
        log_warning(LOGGER_CPU, "Error: Registro inválido en write_mem.");
        return;
    }

    uint32_t direccion_fisica = traducir_direccion(*reg_dire, pcb_actual->PID);
    if (direccion_fisica == SEGMENTATION_FAULT) {
        enviar_interrupcion_segfault(pcb_actual->PID, socket_cpu_memoria);
        return;
    }

    enviar_valor_a_memoria(socket_cpu_memoria, direccion_fisica, reg_dat);

    log_info(LOGGER_CPU, "## TID: <%d> - Accion: <ESCRIBIR> - Direccion Fisica: <%d>", hilo_actual->TID, direccion_fisica);
}

//Suma al Registro Destino el Registro Origen y deja el resultado en el Registro Destino.
void sum_registros(char* destino, char* origen) {
    uint32_t *dest = obtener_registro(destino);
    uint32_t *orig = obtener_registro(origen); 

    if(!dest || !orig) {
        log_warning(LOGGER_CPU, "Error: Registro invalido en sum_registros.");
        return;
    }
    
    *dest += *orig;
}

//Resta al Registro Destino el Registro Origen y deja el resultado en el Registro Destino.
void sub_registros(char* destino, char* origen) {
    uint32_t *dest = obtener_registro(destino);
    uint32_t *orig = obtener_registro(origen); 

    if(!dest || !orig) {
        log_warning(LOGGER_CPU, "Error: Registro invalido en sum_registros.");
        return;
    }
    
    *dest -= *orig;
}

//Si el valor del registro es distinto de cero, actualiza el program counter
//al numero de instruccion pasada por parametro.
void jnz_pc(char* registro, char* instruccion) {
    uint32_t *reg = obtener_registro(registro);

    if(!reg) {
        log_warning(LOGGER_CPU, "Error: Registro inválido en jnz_pc.");
        return;
    }

}

//Escribe en el archivo de log el valor del registro.
void log_registro(char* registro) {
    uint32_t* reg = obtener_registro(registro);

    if (!reg) {
        log_warning(LOGGER_CPU, "Error: Registro inválido '%s' en log_registro.", registro);
        return;
    }

    log_info(LOGGER_CPU, "LOG - Registro %s: %d (PID: %d, PC: %d)", registro, *reg, pcb_actual->PID, hilo_actual->PC);
}