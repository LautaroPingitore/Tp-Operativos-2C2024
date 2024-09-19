#include <include/ciclo_instruccion.h>
#include <include/MMU.h>
#include <include/cpu.h>

//Asigna al registro el valor pasado como parámetro.
void set_registro(char* registro, char *valor)
{
    uint32_t *reg = obtener_registro(registro);

    if (reg) {
        *reg = atoi(valor) // SOLO ACEPTA VALORES NUMERICOS
    } else {
        fprintf(stderr, "Error: Registro invalido en _set.\n");
    }

}

//Lee el valor de memoria correspondiente a
//la Direccion Logica que se encuentra en el Registro Direccion
//y lo almacena en el Registro Datos.
void read_mem(char* reg_datos, char* reg_direccion, int socket) {
    // Paso 1: Obtener los punteros a los registros de datos y direccion
    uint32_t *registro_datos_ptr = obtener_registro(reg_datos);
    uint32_t *registro_direccion_ptr = obtener_registro(reg_direccion);

    // Validacion de los punteros a registros
    if (registro_datos_ptr == NULL || registro_direccion_ptr == NULL) {
        fprintf(stderr, "Error: Registro invalido en read_mem.\n");
        return;
    }

    // Paso 2: Obtener la direccion logica del registro de direccion
    uint32_t direccion_logica = *registro_direccion_ptr;

    // Paso 3: Traducir la direccion logica a fisica usando la MMU
    uint32_t direccion_fisica = traducir_direccion(direccion_logica, pcb_actual->pid);

    // Validar que la traduccion sea correcta
    if (direccion_fisica == -1) {
        fprintf(stderr, "Error: Segmentation Fault en read_mem.\n");
        enviar_interrupcion_segfault(pcb_actual->pid, socket);
        return;
    }

    // Paso 4: Solicitar a memoria el valor en la direccion fisica
    uint32_t valor_leido = pedir_valor_memoria(direccion_fisica, socket);

    // Paso 5: Almacenar el valor leido en el registro de datos
    *registro_datos_ptr = valor_leido;

    // Paso 6: Loguear la accion
    log_info(LOGGER_CPU, "PID: %d - Accion: LEER - Direccion Fisica: %d - Valor: %d", 
             pcb_actual->pid, direccion_fisica, valor_leido);

    free(valor_leido);
}





































//Lee el valor del Registro Datos y lo escribe en la 
//direccion fisica de memoria obtenida a partir de la 
//Direccion Logica almacenada en el Registro Direccion.
void write_mem(char* direccion_logica)
{

}

//Suma al Registro Destino el Registro Origen y deja el resultado en el Registro Destino.
void sum_registros(char* destino, char* origen)
{

}

//Resta al Registro Destino el Registro Origen y deja el resultado en el Registro Destino.
void sub_registros(char* destino, char* origen)
{

}

//Si el valor del registro es distinto de cero, actualiza el program counter
//al número de instruccion pasada por parámetro.
void jnz_pc(char* registro, char* instruccion)
{

}

//Escribe en el archivo de log el valor del registro.
void log_registro(char* registro)
{

}

uint32* obtener_registro(char* registro){

    if(strcmp(registro,"AX")==0)
        return &(pcb_actual->contexto_ejecucion->registros->AX);
    if(strcmp(registro,"BX")==0)
        return &(pcb_actual->contexto_ejecucion->registros->BX);
    if(strcmp(registro,"CX")==0)
        return &(pcb_actual->contexto_ejecucion->registros->CX);
    if(strcmp(registro,"DX")==0)
        return &(pcb_actual->contexto_ejecucion->registros->DX);
    if(strcmp(registro,"EX")==0)
        return &(pcb_actual->contexto_ejecucion->registros->EX);
    if(strcmp(registro,"GX")==0)
        return &(pcb_actual->contexto_ejecucion->registros->GX);
    if(strcmp(registro,"HX")==0)
        return &(pcb_actual->contexto_ejecucion->registros->HX);
    if(strcmp(registro,"PC")==0)
        return &(pcb_actual->contexto_ejecucion->registros->PC);
}

// PUEDE ESTAR EN MEMORIA
uint32_t str_to_uint32(char *str)
{
    char *endptr;
    uint32_t result = (uint32_t)strtoul(str, &endptr, 10);

    // Comprobar si hubo errores durante la conversion
    if (*endptr != '\0')
    {
        fprintf(stderr, "Error en la conversion de '%s' a uint32_t.\n", str);
        exit(EXIT_FAILURE);
    }
}