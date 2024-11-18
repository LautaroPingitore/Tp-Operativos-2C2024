#ifndef GESTION_MEMORIA_H
#define GESTION_MEMORIA_H

extern t_list* lista_particiones;

typedef struct {
    uint32_t inicio;
    uint32_t tamano;
    bool libre;
} t_particion;

// Funciones para manejar particiones y memoria 
void inicializar_lista_particiones(char* esquema, t_list* particiones_fijas);
t_particion* buscar_hueco(uint32_t tamano_requerido, const char* algoritmo);
t_particion* buscar_hueco_first_fit(uint32_t tamano_requerido);
t_particion* buscar_hueco_best_fit(uint32_t tamano_requerido);
t_particion* buscar_hueco_worst_fit(uint32_t tamano_requerido);
int asignar_espacio_memoria(t_proceso_memoria* proceso, const char* algoritmo);
void liberar_espacio_memoria(t_proceso_memoria* proceso);
void consolidar_particiones_libres();

#endif