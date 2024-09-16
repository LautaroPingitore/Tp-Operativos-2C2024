#include <include/ciclo_instruccion.h>
#include <include/MMU.h>

// Lista de particiones asignadas a procesos (podria ser una lista global o en memoria compartida)
t_list *lista_particiones;


uint32_t obtener_base_particion(uint32_t pid) { // CHAT GPT LAU
    // Recorremos la lista de particiones para encontrar la que corresponde al PID
    for (int i = 0; i < list_size(lista_particiones); i++) {
        t_particion *particion = list_get(lista_particiones, i);
        if (particion->pid == pid) {
            return particion->base;
        }
    }

    // Si no se encuentra la particion, registramos un error y salimos
    log_error(LOGGER_CPU, "PID: %d - No se encontro la particion asignada.", pid);
    exit(EXIT_FAILURE);  // Podrias manejar esto de forma mas robusta
}

uint32_t obtener_limite_particion(uint32_t pid) {
    // Recorremos la lista de particiones para encontrar la que corresponde al PID
    for (int i = 0; i < list_size(lista_particiones); i++) {
        t_particion *particion = list_get(lista_particiones, i);
        if (particion->pid == pid) {
            return particion->limite;
        }
    }

    // Si no se encuentra la particion, registramos un error y salimos
    log_error(LOGGER_CPU, "PID: %d - No se encontro el limite de la particion asignada.", pid);
    exit(EXIT_FAILURE);  // Podrias manejar esto de forma mas robusta
}


t_list *traducir_direccion(uint32_t pid, uint32_t logicalAddress, uint32_t tamanio_registro)
{
    // Crear la lista que almacenara las direcciones fisicas
    t_list *listaDirecciones = list_create();
    t_direcciones_fisicas *direccion = malloc(sizeof(t_direcciones_fisicas));

    // Obtener la base de la particion asignada al proceso con pid
    uint32_t base_particion = obtener_base_particion(pid);

    // Verificar si se encontro una particion valida
    if (base_particion == -1) {
        log_error(LOGGER_CPU, "PID: %d - No se encontro una particion asignada.", pid);
        return listaDirecciones; // Retornar una lista vacia en caso de error
    }

    // Obtener el limite de la particion para verificar si el acceso es valido
    uint32_t limite_particion = obtener_limite_particion(pid);

    // Calcular la direccion fisica sumando la direccion base y la direccion logica
    uint32_t direccion_fisica = base_particion + logicalAddress;

    // Verificar si la direccion logica esta dentro de los limites de la particion
    if (logicalAddress + tamanio_registro > limite_particion) {
        free(direccion);
        log_error(LOGGER_CPU, "PID: %d - Direccion fuera de los limites de la particion.", pid);
        return listaDirecciones; // Retornar lista vacia en caso de error
    }

    // Calcular el tamanio que se puede leer/escribir dentro de esta particion
    direccion->direccion_fisica = direccion_fisica;
    direccion->tamanio = tamanio_registro;

    // Aniadir la direccion fisica a la lista de direcciones
    list_add(listaDirecciones, direccion);

    // Si ocupa mas de una particion (esto es improbable en particiones, pero podria ocurrir en un sistema dinamico),
    // manejarlo adecuadamente (aqui no es necesario ya que no tenemos paginacion).

    return listaDirecciones;
}