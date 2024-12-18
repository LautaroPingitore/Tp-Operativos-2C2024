#ifndef COMUNICACIONESCPU_H_
#define COMUNICACIONESCPU_H_

extern bool hay_interrupcion;

// CICLO_INSTRUCCION
t_tcb* recibir_hilo_kernel(int);
void pedir_contexto_memoria(int, uint32_t, uint32_t);
void* recibir_interrupcion(void*);
bool deserializar_interrupcion(void*, int, int, bool);
void pedir_instruccion_memoria(uint32_t, uint32_t, uint32_t, int);
void enviar_contexto_memoria(uint32_t, uint32_t, t_registros*, int);
void enviar_syscall_kernel(t_instruccion*, op_code);
t_instruccion* recibir_instruccion(int);
void devolver_control_al_kernel();

// INSTRUCCIONES
void enviar_interrupcion_segfault(uint32_t, int);
void enviar_valor_a_memoria(int, uint32_t, uint32_t*);
void enviar_solicitud_valor_memoria(int, uint32_t);
uint32_t recibir_valor_de_memoria(int);

// MMU
void enviar_solicitud_memoria(int, uint32_t, op_code, const char*);
uint32_t recibir_entero(int, const char*);

t_proceso_cpu* recibir_proceso(int);
t_proceso_cpu* deserializar_proceso(t_buffer*);

#endif //COMUNICACIONESCPU_H_