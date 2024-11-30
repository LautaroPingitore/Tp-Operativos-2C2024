#ifndef COMUNICACIONESCPU_H_
#define COMUNICACIONESCPU_H_

// CICLO_INSTRUCCION
t_tcb* recibir_hilo_kernel(int);
bool recibir_interrupcion(int);
bool deserializar_interrupcion(void*, int, int, bool);
void pedir_instruccion_memoria(uint32_t, uint32_t, int);
void enviar_contexto_memoria(uint32_t, uint32_t, t_registros*, uint32_t, int);
void enviar_syscall_kernel(t_instruccion*, op_code);

// INSTRUCCIONES
void enviar_interrupcion_segfault(uint32_t, int);
void enviar_valor_a_memoria(int, uint32_t, uint32_t);
void enviar_solicitud_valor_memoria(int, uint32_t);
void recibir_valor_de_memoria(int, uint32_t, uint32_t);

// MMU
void enviar_solicitud_memoria(uint32_t, op_code, const char*);
uint32_t recibir_entero(int, const char*);

#endif //COMUNICACIONESCPU_H_