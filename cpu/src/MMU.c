#include <include/ciclo_instruccion.h>
#include <include/MMU.h>
#define SEGMENTATION_FAULT ((uint32_t)-1)

// MMU: Traducir direccion logica a fisica con Asignacion Contigua
uint32_t traducir_direccion(uint32_t logicalAddress, uint32_t pid) {
    uint32_t base = consultar_base_particion(pid);
    uint32_t limite = consultar_limite_particion(pid);

    if (logicalAddress > limite) {
        log_error(LOGGER_CPU, "Segmentation Fault: PID: %d, Direccion logica: %d", pid, logicalAddress);
        return SEGMENTATION_FAULT;
    }

    uint32_t direccion_fisica = base + logicalAddress;
    log_info(LOGGER_CPU, "Traduccion Direccion Logica: %d a Fisica: %d", logicalAddress, direccion_fisica);
    return direccion_fisica;
}

// A la hora de ejecutar instrucciones que requieran interactuar directamente con la Memoria,
//tendra que traducir las direcciones logicas (propias del proceso) a direcciones fisicas (propias de la memoria).
//Para ello simulara la existencia de una MMU

// DEBIDO A ESO ES PROBABLE QUE LA BASE Y LIMITE DE LA PARTICION SE DEBA CONSULTAR AL MODULO MEMORIA
uint32_t consultar_base_particion(uint32_t pid) {
    enviar_solicitud_base_memoria(pid);

    uint32_t base_particion = recibir_base_memoria();

    if (base_particion == (uint32_t)-1) { // Usa (uint32_t)-1 en lugar de -1 para asegurar la comparacion con uint32_t
        log_error(LOGGER_CPU, "Error: No se encontro la particion para el PID: %d", pid);
        return (uint32_t)-1; // Usa (uint32_t)-1 para indicar error
    }

    log_info(LOGGER_CPU, "Base de la particion obtenida para PID: %d - Base: %d", pid, base_particion);
    return base_particion;
}

uint32_t consultar_limite_particion(uint32_t pid) {
    enviar_solicitud_limite_memoria(pid);

    uint32_t limite_particion = recibir_limite_memoria();

    if (limite_particion == (uint32_t)-1) { // Usa (uint32_t)-1 en lugar de -1 para asegurar la comparacion con uint32_t
        log_error(LOGGER_CPU, "Error: No se encontro la particion para el PID: %d", pid);
        return (uint32_t)-1; // Usa (uint32_t)-1 para indicar error
    }

    log_info(LOGGER_CPU, "Limite de la particion obtenida para PID: %d - Limite: %d", pid, limite_particion);
    return limite_particion;
}

//FUNCIONES DECLARADAS A DESARROLLAR
void enviar_solicitud_base_memoria(uint32_t pid){
    t_paquete* paquete = crear_paquete_con_codigo_de_operacion(SOLICITUD_BASE_MEMORIA);

    // paquete->codigo_operacion = SOLUCITUD_BASE_MEMORIA;

    agregar_a_paquete(paquete, &pid, sizeof(uint32_t));

    enviar_paquete(paquete, socket_cpu_dispatch_memoria);

    eliminar_paquete(paquete);
}

uint32_t recibir_base_memoria(){
    uint32_t base_particion;

    ssize_t bytes_recibidos = recv(socket_cpu_dispatch_memoria, &base_particion, sizeof(uint32_t), MSG_WAITALL);

    if (bytes_recibidos <= 0) {
        log_error(LOGGER_CPU, "Error al recibir la base de la particion desde memoria.");
        return (uint32_t) -1;
    }

    return base_particion;
}

void enviar_solicitud_limite_memoria(uint32_t pid){
    t_paquete* paquete = crear_paquete_con_codigo_de_operacion(SOLICITUD_LIMITE_MEMORIA);

    //paquete->codigo_operacion = SOLICITUD_LIMITE_MEMORIA;

    agregar_a_paquete(paquete, &pid, sizeof(uint32_t));

    enviar_paquete(paquete, socket_cpu_dispatch_memoria);

    eliminar_paquete(paquete);
}

uint32_t recibir_limite_memoria(){
    uint32_t base_particion;

    ssize_t bytes_recibidos = recv(socket_cpu_dispatch_memoria, &base_particion, sizeof(uint32_t), MSG_WAITALL);

    if (bytes_recibidos <= 0) {
        log_error(LOGGER_CPU, "Error al recibir la base de la particion desde memoria.");
        return (uint32_t) -1;
    }

    return base_particion;
}







































// Lista de particiones asignadas a procesos (podria ser una lista global o en memoria compartida)
//t_list *lista_particiones;
// uint32_t obtener_base_particion(uint32_t pid) { // CHAT GPT LAU
//     // Recorremos la lista de particiones para encontrar la que corresponde al PID
//     for (int i = 0; i < list_size(lista_particiones); i++) {
//         t_particion *particion = list_get(lista_particiones, i);
//         if (particion->pid == pid) {
//             return particion->base;
//         }
//     }


//     // Si no se encuentra la particion, registramos un error y salimos
//     log_error(LOGGER_CPU, "PID: %d - No se encontro la particion asignada.", pid);
//     exit(EXIT_FAILURE);  // Podrias manejar esto de forma mas robusta
// }

// uint32_t obtener_limite_particion(uint32_t pid) {
//     // Recorremos la lista de particiones para encontrar la que corresponde al PID
//     for (int i = 0; i < list_size(lista_particiones); i++) {
//         t_particion *particion = list_get(lista_particiones, i);
//         if (particion->pid == pid) {
//             return particion->limite;
//         }
//     }

//     // Si no se encuentra la particion, registramos un error y salimos
//     log_error(LOGGER_CPU, "PID: %d - No se encontro el limite de la particion asignada.", pid);
//     exit(EXIT_FAILURE);  // Podrias manejar esto de forma mas robusta
// }


// t_list *traducir_direccion(uint32_t pid, uint32_t logicalAddress, uint32_t tamanio_registro)
// {
//     // Crear la lista que almacenara las direcciones fisicas
//     t_list *listaDirecciones = list_create();
//     t_direcciones_fisicas *direccion = malloc(sizeof(t_direcciones_fisicas));

//     // Obtener la base de la particion asignada al proceso con pid
//     uint32_t base_particion = obtener_base_particion(pid);

//     // Verificar si se encontro una particion valida
//     if (base_particion == -1) {
//         log_error(LOGGER_CPU, "PID: %d - No se encontro una particion asignada.", pid);
//         return listaDirecciones; // Retornar una lista vacia en caso de error
//     }

//     // Obtener el limite de la particion para verificar si el acceso es valido
//     uint32_t limite_particion = obtener_limite_particion(pid);

//     // Calcular la direccion fisica sumando la direccion base y la direccion logica
//     uint32_t direccion_fisica = base_particion + logicalAddress;

//     // Verificar si la direccion logica esta dentro de los limites de la particion
//     if (logicalAddress + tamanio_registro > limite_particion) {
//         free(direccion);
//         log_error(LOGGER_CPU, "PID: %d - Direccion fuera de los limites de la particion.", pid);
//         return listaDirecciones; // Retornar lista vacia en caso de error
//     }

//     // Calcular el tamanio que se puede leer/escribir dentro de esta particion
//     direccion->direccion_fisica = direccion_fisica;
//     direccion->tamanio = tamanio_registro;

//     // Aniadir la direccion fisica a la lista de direcciones
//     list_add(listaDirecciones, direccion);

//     // Si ocupa mas de una particion (esto es improbable en particiones, pero podria ocurrir en un sistema dinamico),
//     // manejarlo adecuadamente (aqui no es necesario ya que no tenemos paginacion).

//     return listaDirecciones;
// }