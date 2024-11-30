#ifndef COMUNICACIONESCPU_H_
#define COMUNICACIONESCPU_H_

t_tcb* recibir_hilo_kernel(int);
bool recibir_interrupcion(int);
bool deserializar_interrupcion(void*, int, int, bool);
void pedir_instruccion_memoria(uint32_t, uint32_t, int);
void enviar_contexto_memoria(uint32_t, uint32_t, t_registros*, uint32_t, int);
void enviar_syscall_kernel(t_instruccion*, op_code);

#endif //COMUNICACIONESCPU_H_