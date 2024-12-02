#include "include/gestor.h"

#define SEGMENTATION_FAULT ((uint32_t)-1)
#define BASE_ERROR ((uint32_t)-1)
#define LIMITE_ERROR ((uint32_t)-1)

// MMU: Traducir direccion logica a fisica con Asignacion Contigua
uint32_t traducir_direccion(uint32_t direccion_logica, uint32_t pid) {
    uint32_t base = consultar_base_particion(pid);
    if (base == BASE_ERROR) {
        log_error(LOGGER_CPU, "Error al consultar la base para el PID: %d", pid);
        return SEGMENTATION_FAULT;
    }

    uint32_t limite = consultar_limite_particion(pid);
    if (limite == LIMITE_ERROR) {
        log_error(LOGGER_CPU, "Error al consultar el límite para el PID: %d", pid);
        return SEGMENTATION_FAULT;
    }
    
    if (direccion_logica > limite) {
        log_error(LOGGER_CPU, "Segmentation Fault: PID: %d, Direccion logica: %d", pid, direccion_logica);
        return SEGMENTATION_FAULT;
    }

    uint32_t direccion_fisica = base + direccion_logica;
    log_info(LOGGER_CPU, "Traduccion Direccion Logica: %d a Fisica: %d", direccion_logica, direccion_fisica);
    return direccion_fisica;
}

// DEBIDO A ESO ES PROBABLE QUE LA BASE Y LIMITE DE LA PARTICION SE DEBA CONSULTAR AL MODULO MEMORIA
uint32_t consultar_base_particion(uint32_t pid) {
    enviar_solicitud_memoria(socket_cpu_memoria, pid, SOLICITUD_BASE_MEMORIA, "base de particion");
    
    uint32_t base_particion = recibir_entero(socket_cpu_memoria, "Base de particion");
    if (base_particion == BASE_ERROR) {
        log_error(LOGGER_CPU, "No se encontró la base de la partición para el PID: %d", pid);
    }

    log_info(LOGGER_CPU, "Base de la particion obtenida para PID: %d - Base: %d", pid, base_particion);
    return base_particion;
}

uint32_t consultar_limite_particion(uint32_t pid) {
    enviar_solicitud_memoria(socket_cpu_memoria, pid, SOLICITUD_LIMITE_MEMORIA, "limite de particion");

    uint32_t limite = recibir_entero(socket_cpu_memoria, "Límite de partición");
    if (limite == LIMITE_ERROR) {
        log_error(LOGGER_CPU, "No se encontró el límite de la partición para el PID: %d", pid);
    }

    log_info(LOGGER_CPU, "Limite de la particion obtenida para PID: %d - Limite: %d", pid, limite);
    return limite;
}