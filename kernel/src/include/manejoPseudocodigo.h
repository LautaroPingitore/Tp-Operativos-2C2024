#ifndef MANEJOPSEUDOCODIGO_H_
#define MANEJOPSEUDOCODIGO_H_

#include <utils/hello.h>
#include <include/kernel.h>
#include <include/planificador.h>
#include <include/syscall.h>

typedef struct {
    instruccion intrucciones[100];
    int cantidad_instrucciones;
} archivo_pseudocodigo;

typedef struct {
    char comando[50];
    char argumento1[50];
    char argumento2[50];
} instruccion;

typedef struct {
    char nombre[50];
    bool es_mutex;
} recurso;

typedef struct {
    recurso recursos[10];
    int cantidad_recursos;
} lista_recursos;

int leer_archivo_pseudocodigo(const char*, archivo_pseudocodigo *);
int obtener_recursos_del_proceso(const archivo_pseudocodigo *, lista_recursos *);

#endif /* MANEJOPSEUDOCODIGO_H_ */