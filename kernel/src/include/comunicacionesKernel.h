#ifndef COMUNICACIONESKERNEL_H_
#define COMUNICACIONESKERNEL_H_

void enviar_proceso_memoria(int, t_pcb*, op_code);
int respuesta_memoria_creacion(int);
void envio_hilo_crear(int, t_tcb*, op_code);
int enviar_hilo_a_cpu(t_tcb*, char*);
t_tcb* recibir_hilo_cpu();

#endif /* COMUNICACIONESKERNEL_H_ */