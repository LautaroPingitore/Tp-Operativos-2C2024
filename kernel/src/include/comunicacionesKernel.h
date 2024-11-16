#ifndef COMUNICACIONESKERNEL_H_
#define COMUNICACIONESKERNEL_H_

void enviar_proceso_memoria(int, t_pcb*, op_code);
int respuesta_memoria_creacion(int);
void envio_hilo_crear(int, t_tcb*, op_code);

#endif /* COMUNICACIONESKERNEL_H_ */