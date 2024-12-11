#ifndef GESTION_MEMORIA_H
#define GESTION_MEMORIA_H

extern t_list* lista_particiones;

typedef struct {
    uint32_t inicio;
    uint32_t tamano;
    bool libre;
} t_particion;

// Funciones para manejar particiones y memoria 
void inicializar_lista_particiones(char*, t_list*);
t_particion* buscar_hueco(uint32_t, const char*);
t_particion* buscar_hueco_first_fit(uint32_t);
t_particion* buscar_hueco_best_fit(uint32_t);
t_particion* buscar_hueco_worst_fit(uint32_t);
int asignar_espacio_memoria(t_proceso_memoria*, const char*);
void liberar_espacio_memoria(uint32_t);
void consolidar_particiones_libres();

#endif