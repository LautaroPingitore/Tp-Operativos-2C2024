#ifndef CICLO_INSTRUCCION_H_
#define CICLO_INSTRUCCION_H_

#include "../../../kernel/src/include/planificador.h"

extern t_tcb* hilo_actual;

typedef struct {
    uint32_t direccion_fisica;
    uint32_t tamanio;
} t_direcciones_fisicas;

void ejecutar_ciclo_instruccion(int);
t_instruccion *fetch(uint32_t, uint32_t);
void execute(t_instruccion *, int);
void check_interrupt();


void loguear_y_sumar_pc(t_instruccion*);

void liberar_instruccion(t_instruccion*);

void actualizar_contexto_memoria();

void devolver_control_al_kernel();

#endif //CICLO_INSTRUCCION_H_