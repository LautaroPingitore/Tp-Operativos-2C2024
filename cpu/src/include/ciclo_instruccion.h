#ifndef CICLO_INSTRUCCION_H_
#define CICLO_INSTRUCCION_H_

typedef struct {
    uint32_t direccion_fisica;
    uint32_t tamanio;
} t_direcciones_fisicas;

void ejecutar_ciclo_instruccion(int);
t_instruccion *fetch(uint32_t, uint32_t);
void execute(t_instruccion *, int);
void check_interrupt();


void loguear_y_sumar_pc(t_instruccion*);

void pedir_instruccion_memoria(uint32_t, uint32_t, int);
t_instruccion *deserializar_instruccion(int);
void liberar_instruccion(t_instruccion*);

bool recibir_interrupcion(int);
void actualizar_contexto_memoria();

void enviar_contexto_memoria(uint32_t, t_registros*, uint32_t, int);
void devolver_control_al_kernel();

#endif //CICLO_INSTRUCCION_H_