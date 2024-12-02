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
    t_registros* regs = pcb_actual->CONTEXTO->registros;

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

    if (!reg) {
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
void read_mem(char* reg_datos, char* reg_direccion, int socket) {
    // Obtener los punteros a los registros
    uint32_t *registro_datos = obtener_registro(reg_datos);
    uint32_t *registro_direccion = obtener_registro(reg_direccion);

    // Validar que los registros sean validos
    if (!registro_datos || !registro_direccion) {
        log_warning(LOGGER_CPU, "Error: Registro invalido en read_mem.");
        return;
    }

    // Traducir la direccion logica a direccion fisica
    uint32_t direccion_fisica = traducir_direccion(*registro_direccion, pcb_actual->PID);

    // Verificar si la traduccion resulto en un Segmentation Fault
    if (direccion_fisica == SEGMENTATION_FAULT) {
        enviar_interrupcion_segfault(pcb_actual->PID, socket);
        return;
    }

    enviar_solicitud_valor_memoria(socket, direccion_fisica);
    uint32_t valor_leido;

    // Leer el valor de memoria desde la direccion fisica y se lo asigna al valor leido
    recibir_valor_de_memoria(socket, valor_leido, direccion_fisica);
    if (!valor_leido) {
        log_warning(LOGGER_CPU, "Error al leer memoria en read_mem.");
        return;
    }

    // Almacenar el valor en el registro de datos
    *registro_datos = valor_leido;

    // Loguear la accion de lectura
    log_info(LOGGER_CPU, "PID: %d - Leer Memoria - Direccion Fisica: %d - Valor: %d",
             pcb_actual->PID, direccion_fisica, valor_leido);
}

//Lee el valor del Registro Datos y lo escribe en la 
//direccion fisica de memoria obtenida a partir de la 
//Direccion Logica almacenada en el Registro Direccion.
void write_mem(char* reg_direccion, char* reg_datos, int socket) {
    // Obtener los punteros a los registros
    uint32_t *reg_dire = obtener_registro(reg_direccion);
    uint32_t *reg_dat = obtener_registro(reg_datos);

    // Validar que los registros sean validos
    if (!reg_dire || !reg_dat) {
        log_warning(LOGGER_CPU, "Error: Registro invalido en write_mem.");
        return;
    }

    // Traducir la direccion logica a direccion fisica
    uint32_t direccion_fisica = traducir_direccion(*reg_dire, pcb_actual->PID);
    
    // Verificar si la traduccion resulto en un Segmentation Fault
    if (direccion_fisica == SEGMENTATION_FAULT) {
        enviar_interrupcion_segfault(pcb_actual->PID, socket);
        return;
    }

    // Enviar el valor a la memoria para escribir en la direccion fisica
    enviar_valor_a_memoria(socket, direccion_fisica, reg_dat);

    // Loguear la accion de escritura
    log_info(LOGGER_CPU, "PID: %d - Escribir Memoria - Direccion Fisica: %d - Valor: %d",
             pcb_actual->PID, direccion_fisica, *reg_dat);
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