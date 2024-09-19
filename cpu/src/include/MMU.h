#ifndef MMU_H_
#define MMU_H_

#include <include/cpu.h>

typedef struct {
    uint32_t pid;      // Identificador del proceso
    uint32_t base;     // Direccion base de la particion asignada
    uint32_t limite;   // Tamanio o limite de la particion asignada
} t_particion;

// typedef struct {
//     uint32_t direccion_fisica;
//     uint32_t tamanio;
// } t_direcciones_fisicas;

uint32_t consultar_base_particion(uint32_t);
uint32_t consultar_limite_particion(uint32_t);
uint32_t traducir_direccion(uint32_t, uint32_t);

#endif /* MMU_H_ */