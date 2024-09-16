#ifndef INSTRUCCIONES_H_
#define INSTRUCCIONES_H_

#include <include/cpu.h>

void set_registro(char*, char*);
void read_mem(char*, char*, int);
void write_mem(char*, char*, int);
void sum_registros(char*, char*);
void sub_registros(char*, char*);
void jnz_pc(char*, char*);
void log_registro(char*);