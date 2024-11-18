#include "include/gestor.h"

#define VALOR_ERROR ((uint32_t)-1)

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
    if (strcmp(registro, "PC") == 0) return &regs->program_counter;

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

    // Leer el valor de memoria desde la direccion fisica
    uint32_t valor_leido = recibir_valor_de_memoria(socket, direccion_fisica);
    if (valor == (uint32_t)-1) {
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
    enviar_valor_a_memoria(socket, direccion_fisica, *reg_dat);

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

//Si el valor del registro es distinto de cero, actualiza el program counter
//al numero de instruccion pasada por parametro.
void jnz_pc(char* registro, char* instruccion) {
    uint32_t *reg = obtener_registro(registro);

    if(!reg) {
        log_warning(LOGGER_CPU, "Error: Registro inválido en jnz_pc.");
        return;
    }

    if(*reg != 0) {
        pcb_actual->CONTEXTO->registros->program_counter = atoi(instruccion);
    }
}

//Escribe en el archivo de log el valor del registro.
void log_registro(char* registro) {
    uint32_t* reg = obtener_registro(registro);

    if (!reg) {
        log_warning(LOGGER_CPU, "Error: Registro inválido '%s' en log_registro.", registro);
        return;
    }

    log_info(LOGGER_CPU, "LOG - Registro %s: %d (PID: %d, PC: %d)", registro, *reg, pcb_actual->PID, pcb_actual->CONTEXTO->registros->program_counter);
}



//FUNCIONES DE ABAJO A REVISAR:
void enviar_interrupcion_segfault(uint32_t pid, int socket) {
    // Crear el paquete de interrupcion
    t_paquete* paquete = crear_paquete_con_codigo_de_operacion(SEGF_FAULT);
    
    // Agregar el PID al paquete
    agregar_a_paquete(paquete, &pid, sizeof(uint32_t));
    
    // Enviar el paquete al socket de la memoria o kernel
    if (enviar_paquete(paquete, socket) < 0) {
        log_error(LOGGER_CPU, "Error al enviar interrupción de Segmentation Fault para PID: %d", pid);
    } else {
        log_info(LOGGER_CPU, "Interrupción de Segmentation Fault enviada correctamente (PID: %d)", pid);
    }
    
    // Liberar el paquete
    eliminar_paquete(paquete);
}

uint32_t recibir_valor_de_memoria(int socket_cliente, uint32_t direccion_fisica) {
    if (socket < 0) {
        log_error(LOGGER_CPU, "Socket inválido al intentar recibir valor de memoria.");
        return VALOR_ERROR;
    }

    t_paquete *paquete = crear_paquete_con_codigo_de_operacion(PEDIR_VALOR_MEMORIA);
    agregar_a_paquete(paquete, &direccion_fisica, sizeof(uint32_t));

    if (enviar_paquete(paquete, socket) < 0) {
        log_error(LOGGER_CPU, "Error al enviar solicitud de valor desde memoria.");
        eliminar_paquete(paquete);
        return VALOR_ERROR;
    }

    eliminar_paquete(paquete);

    uint32_t valor_leido;
    ssize_t bytes_recibidos = recv(socket_cliente, &valor_leido, sizeof(uint32_t), MSG_WAITALL); 
    // ssize_t es un tipo de dato utilizado para almacenar el número de bytes recibidos por recv
    // MSG_WAITALL indica que la función no debe retornar hasta haber recibido exactamente la cantidad de bytes solicitados en sizeof

    if(bytes_recibidos <= 0) {
        log_error(LOGGER_CPU, "Error al recibir valor desde memoria en la direccion fisica %d.", direccion_fisica);
        return VALOR_ERROR;
    }

    log_info(LOGGER_CPU, "Valor recibido de Memoria: Dirección Física %d - Valor %d", direccion_fisica, valor_leido);
    return valor_leido;
}

void enviar_valor_a_memoria(int socket_cliente, uint32_t dire_fisica, uint32_t valor) {
    if (socket < 0) {
        log_error(LOGGER_CPU, "Socket inválido al intentar enviar valor a memoria.");
        return;
    }

    t_paquete *paquete = crear_paquete_con_codigo_de_operacion(ESCRIBIR_VALOR_MEMORIA);

    // Agregar la dirección física y el valor del registro de datos al paquete
    agregar_a_paquete(paquete, &dire_fisica, sizeof(uint32_t));
    agregar_a_paquete(paquete, &valor, sizeof(uint32_t));

    if (enviar_paquete(paquete, socket) < 0) {
        log_error(LOGGER_CPU, "Error al enviar valor a memoria: Dirección Física %d - Valor %d", direccion_fisica, valor);
    } else {
        log_info(LOGGER_CPU, "Valor enviado a Memoria: Dirección Física %d - Valor %d", direccion_fisica, valor);
    }
    
    // Eliminar el paquete para liberar memoria
    eliminar_paquete(paquete);
}
