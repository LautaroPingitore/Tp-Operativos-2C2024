#ifndef GESTION_MEMORIA_H
#define GESTION_MEMORIA_H

extern t_list* lista_particiones;
extern void* memoria_sistema;

extern pthread_mutex_t mutex_particiones;

typedef struct {
    uint32_t inicio;
    uint32_t tamano;
    bool libre;
} t_particion;

// Funciones para manejar particiones y memoria 
void inicializar_lista_particiones(char*, t_list*);
void inicializar_memoria_sistema();
uint32_t buscar_base_registro(uint32_t, uint32_t);
t_particion* buscar_hueco(uint32_t, const char*);
t_particion* buscar_hueco_first_fit(uint32_t);
t_particion* buscar_hueco_best_fit(uint32_t);
t_particion* buscar_hueco_worst_fit(uint32_t);
int asignar_espacio_memoria(t_proceso_memoria*, const char*);
int liberar_espacio_memoria(uint32_t);
void consolidar_particiones_libres();
t_particion* dividir_particion(t_particion*, uint32_t, uint32_t);
bool es_fija();
int buscar_posicion_particion(t_particion*);


#endif