#include "include/gestor.h"

#define SEGMENTATION_FAULT ((uint32_t)-1)
#define BASE_ERROR ((uint32_t)-1)
#define LIMITE_ERROR ((uint32_t)-1)

// MMU: Traducir direccion logica a fisica con Asignacion Contigua
uint32_t traducir_direccion(uint32_t direccion_logica, uint32_t pid) {
    enviar_solicitud_base_memoria(socket_cpu_memoria, pid, direccion_logica);

    sem_wait(&sem_base);
    uint32_t base = base_pedida;
    if (base == BASE_ERROR) {
        log_error(LOGGER_CPU, "Error al consultar la base para el PID: %d", pid);
        return SEGMENTATION_FAULT;
    }

    enviar_solicitud_limite_memoria(socket_cpu_memoria, pid, direccion_logica);

    sem_wait(&sem_limite);
    uint32_t limite = limite_pedido;
    if (limite == LIMITE_ERROR) {
        log_error(LOGGER_CPU, "Error al consultar el límite para el PID: %d", pid);
        return SEGMENTATION_FAULT;
    }

    uint32_t direccion_fisica = base;
    log_info(LOGGER_CPU, "Traducción Dirección Lógica: %d a Física: %d", direccion_logica, direccion_fisica);
    return direccion_fisica;
}
