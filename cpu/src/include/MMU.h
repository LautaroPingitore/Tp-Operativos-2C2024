#ifndef MMU_H_
#define MMU_H_

typedef struct {
    uint32_t pid;      // Identificador del proceso
    uint32_t base;     // Direccion base de la particion asignada
    uint32_t limite;   // Tamanio o limite de la particion asignada
} t_particion;

uint32_t traducir_direccion(uint32_t, uint32_t);
uint32_t consultar_base_particion(uint32_t);
uint32_t consultar_limite_particion(uint32_t);

void enviar_solicitud_memoria(uint32_t, op_code, const char*);
uint32_t recibir_entero(int, const char*);

#endif /* MMU_H_ */