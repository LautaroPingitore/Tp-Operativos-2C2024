#ifndef MANEJOPSEUDOCODIGO_H_
#define MANEJOPSEUDOCODIGO_H_

#include <utils/hello.h>
#include <include/kernel.h>
#include <include/planificador.h>
#include <include/syscall.h>

typedef struct {
    char* path_archivo;
    t_instruccion* intrucciones;
    int cantidad_instrucciones;
} archivo_pseudocodigo;

typedef struct {
    char* nombre;
    bool es_mutex;
} recurso;

archivo_pseudocodigo* leer_archivo_pseudocodigo(char*);
void asignar_mutex_a_proceso(t_pcb*, char*);
void obtener_recursos_del_proceso(archivo_pseudocodigo, lista_recursos, t_pcb*);
void procesar_archivo(archivo_pseudocodigo);

#endif /* MANEJOPSEUDOCODIGO_H_ */