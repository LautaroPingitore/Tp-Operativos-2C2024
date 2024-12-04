#ifndef MMU_H_
#define MMU_H_

extern uint32_t base_pedida;
extern uint32_t limite_pedido;
extern uint32_t valor_memoria;

extern sem_t sem_base; 
extern sem_t sem_limite;
extern sem_t sem_valor_memoria;
extern sem_t sem_mutex_base_limite;
typedef struct {
    uint32_t pid;      // Identificador del proceso
    uint32_t base;     // Direccion base de la particion asignada
    uint32_t limite;   // Tamanio o limite de la particion asignada
} t_particion;

void iniciar_semaforos();
void destruir_semaforos();
uint32_t traducir_direccion(uint32_t, uint32_t);
uint32_t consultar_base_particion(uint32_t);
uint32_t consultar_limite_particion(uint32_t);

#endif /* MMU_H_ */