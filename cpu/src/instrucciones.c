#include <include/ciclo_instruccion.h>
#include <include/MMU.h>
#include <include/cpu.h>

//Asigna al registro el valor pasado como parámetro.
void set_registro(char* registro, char *valor)
{

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