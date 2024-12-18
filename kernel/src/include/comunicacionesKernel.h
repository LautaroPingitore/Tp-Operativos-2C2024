#ifndef COMUNICACIONESKERNEL_H_
#define COMUNICACIONESKERNEL_H_

extern int respuesta_memoria;

void enviar_proceso_memoria(int, t_pcb*, op_code);
int respuesta_memoria_creacion(int);
void envio_hilo_crear(int, t_tcb*, op_code);
int enviar_hilo_a_cpu(t_tcb*);
void enviar_memory_dump(t_pcb*, uint32_t);
void enviar_interrupcion_cpu(op_code);
void enviar_proceso_cpu(int , t_pcb* );
t_instruccion* recibir_instruccion(int);



#endif /* COMUNICACIONESKERNEL_H_ */