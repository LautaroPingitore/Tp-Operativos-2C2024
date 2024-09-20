#ifndef INSTRUCCIONES_H_
#define INSTRUCCIONES_H_

#include <include/cpu.h>
#include <include/MMU.h>
#include <include/ciclo_instruccion.h>

//instrucciones
void set_registro(char*, char*);
void read_mem(char*, char*, int);
void write_mem(char*, char*, int);
void sum_registros(char*, char*);
void sub_registros(char*, char*);
void jnz_pc(char*, char*);
void log_registro(char*);

//funciones ayuda
void log_registro(char*);
uint32_t* obtener_registro(char*);
void* recibir_dato_de_memoria(int,t_log*,t_list*,uint32_t);
void enviar_interrupcion_segfault(uint32_t, int);