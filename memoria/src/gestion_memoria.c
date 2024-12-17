#include "include/gestor.h"

// Variables Globales
t_list* lista_particiones;
pthread_mutex_t mutex_particiones = PTHREAD_MUTEX_INITIALIZER;

// Inicializa la lista de particiones, en base al esquema elegido (fijo o dinámico)
void inicializar_lista_particiones(char* esquema, t_list* particiones_fijas) {
    pthread_mutex_lock(&mutex_particiones);
    lista_particiones = list_create();

    if (strcmp(esquema, "FIJAS") == 0) {
        for (int i = 0; i < list_size(particiones_fijas); i++) {
            int* tam_particion = list_get(particiones_fijas, i);
            t_particion* particion = malloc(sizeof(t_particion));
            particion->inicio = (i == 0) ? 0 :
                ((t_particion*)list_get(lista_particiones, i - 1))->inicio +
                ((t_particion*)list_get(lista_particiones, i - 1))->tamano;
            particion->tamano = *tam_particion;
            particion->libre = true;
            list_add(lista_particiones, particion);
        }
    } else if (strcmp(esquema, "DINAMICAS") == 0) {
        // En particiones dinámicas, inicia con una única partición libre que ocupa toda la memoria
        t_particion* particion = malloc(sizeof(t_particion));
        particion->inicio = 0;
        particion->tamano = TAM_MEMORIA; // Definir TAM_MEMORIA en configuración
        particion->libre = true;
        list_add(lista_particiones, particion);
    }

    pthread_mutex_unlock(&mutex_particiones);
    log_info(LOGGER_MEMORIA, "Lista de particiones inicializada bajo esquema: %s", esquema);
    log_info(LOGGER_MEMORIA, "Bajo el algoritmo de busqueda: %s", ALGORITMO_BUSQUEDA);
}

// Función general para buscar hueco usando un algoritmo especificado
t_particion* buscar_hueco(uint32_t tamano_requerido, const char* algoritmo) {
    if (strcmp(algoritmo, "FIRST") == 0) {
        return buscar_hueco_first_fit(tamano_requerido);
    } else if (strcmp(algoritmo, "BEST") == 0) {
        return buscar_hueco_best_fit(tamano_requerido);
    } else if (strcmp(algoritmo, "WORST") == 0) {
        return buscar_hueco_worst_fit(tamano_requerido);
    }
    return NULL;
}

// First Fit: Encuentra el primer hueco que cumpla con el tamaño requerido
t_particion* buscar_hueco_first_fit(uint32_t tamano_requerido) {
    for (int i = 0; i < list_size(lista_particiones); i++) {
        t_particion* particion = list_get(lista_particiones, i);
        if (particion && particion->libre && particion->tamano >= tamano_requerido) {
            return particion;
        }
    }
    
    return NULL; // No hay espacio
}

// Best Fit: Encuentra el hueco que más se ajuste al tamaño solicitado
t_particion* buscar_hueco_best_fit(uint32_t tamano_requerido) {
    t_particion* mejor_particion = NULL;
    uint32_t tamano_minimo = UINT32_MAX;

    for (int i = 0; i < list_size(lista_particiones); i++) {
        t_particion* particion = list_get(lista_particiones, i);
        if (particion && particion->libre && particion->tamano >= tamano_requerido) {
            uint32_t espacio_sobrante = particion->tamano - tamano_requerido;
            if (espacio_sobrante < tamano_minimo) {
                mejor_particion = particion;
                tamano_minimo = espacio_sobrante;
            }
        }
    }
    return mejor_particion;
}

// Worst Fit: Encuentra el hueco más grande disponible
t_particion* buscar_hueco_worst_fit(uint32_t tamano_requerido) {
    t_particion* peor_particion = NULL;
    uint32_t tamano_maximo = 0;

    for (int i = 0; i < list_size(lista_particiones); i++) {
        t_particion* particion = list_get(lista_particiones, i);
        if (particion && particion->libre && particion->tamano >= tamano_requerido) {
            uint32_t espacio_libre = particion->tamano - tamano_requerido;
            if (espacio_libre > tamano_maximo) {
                peor_particion = particion;
                tamano_maximo = espacio_libre;
            }
        }
    }

    return peor_particion;
}

// Asigna espacio de memoria a un proceso, usando un algoritmo de búsqueda específico
int asignar_espacio_memoria(t_proceso_memoria* proceso, const char* algoritmo) {
    t_particion* particion = buscar_hueco(proceso->limite, algoritmo);

    if(particion == NULL){
        log_warning(LOGGER_MEMORIA, "No se encontró espacio suficiente para inicializar el proceso %d usando el algoritmo: %s", proceso->pid, algoritmo);
        return -1;
    }
    
    if (particion->tamano < proceso->limite) {
        log_error(LOGGER_MEMORIA, "ERROR CON LA PARTICION DEL proceso PID %d", proceso->pid);
        return -1;
    }


    particion->libre = false;  
    proceso->base = particion->inicio;

    log_info(LOGGER_MEMORIA, "Memoria asignada a PID %d en la dirección %d con límite de %d bytes",
             proceso->pid, proceso->base, particion->tamano);

    return 1;
}

// Consolida particiones adyacentes libres en particiones dinámicas
void consolidar_particiones_libres() {
    pthread_mutex_lock(&mutex_particiones);

    for (int i = 0; i < list_size(lista_particiones) - 1; i++) {
        t_particion* actual = list_get(lista_particiones, i);
        t_particion* siguiente = list_get(lista_particiones, i + 1);

        if (actual->libre && siguiente->libre) {
            actual->tamano += siguiente->tamano;  // Une las dos particiones
            list_remove_and_destroy_element(lista_particiones, i + 1, free);
            i--;  // Revisa de nuevo la posición actual
        }
    }

    pthread_mutex_unlock(&mutex_particiones);
    log_info(LOGGER_MEMORIA, "Particiones libres consolidadas.");
}

void liberar_espacio_memoria(uint32_t pid) {
    t_proceso_memoria* proceso = obtener_proceso_memoria(pid);

    pthread_mutex_lock(&mutex_particiones);
    bool particion_encontrada = false;

    for (int i = 0; i < list_size(lista_particiones); i++) {
        t_particion* particion = list_get(lista_particiones, i);

        if (particion->inicio == proceso->base && !particion->libre) {
            particion->libre = true;
            particion_encontrada = true;

            if (strcmp(ESQUEMA, "FIJAS") == 0) {
                log_info(LOGGER_MEMORIA, "Memoria liberada para PID %u en la dirección %u (Esquema Fijas)", 
                         proceso->pid, proceso->base);
            } else if (strcmp(ESQUEMA, "DINAMICA") == 0) {
                consolidar_particiones_libres();
                log_info(LOGGER_MEMORIA, "Memoria liberada para PID %u en la dirección %u (Esquema Dinámicas)", 
                         proceso->pid, proceso->base);
            }

            break;
        }
    }

    if (!particion_encontrada) {
        log_error(LOGGER_MEMORIA, "No se encontró la partición para PID %u al intentar liberar memoria (Base: %u)", 
                  proceso->pid, proceso->base);
    }
    
    pthread_mutex_unlock(&mutex_particiones);
}
