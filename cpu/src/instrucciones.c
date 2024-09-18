#include <include/ciclo_instruccion.h>
#include <include/MMU.h>
#include <include/cpu.h>

//Asigna al registro el valor pasado como parámetro.
void set_registro(char* registro, char *valor)
{

    *(obtener_registro(registro)) = str_to_uint32(valor);

}

//Lee el valor de memoria correspondiente a
//la Dirección Lógica que se encuentra en el Registro Dirección
//y lo almacena en el Registro Datos.
void read_mem(char* regustro, char* direccion_logica, int socket)
{

}

//Lee el valor del Registro Datos y lo escribe en la 
//dirección física de memoria obtenida a partir de la 
//Dirección Lógica almacenada en el Registro Dirección.
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
//al número de instrucción pasada por parámetro.
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

    // Comprobar si hubo errores durante la conversión
    if (*endptr != '\0')
    {
        fprintf(stderr, "Error en la conversión de '%s' a uint32_t.\n", str);
        exit(EXIT_FAILURE);
    }
}