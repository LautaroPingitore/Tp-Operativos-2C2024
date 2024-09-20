#include <include/instrucciones.h>



//esto problabemente termine declarado en otro arhivo pero lo hago para descartar errores
t_pcb* pcb_actual;



//Asigna al registro el valor pasado como parametro.
void set_registro(char* registro, char *valor)
{
    uint32_t *reg = obtener_registro(registro);

    if (reg) {
        *reg = atoi(valor); // SOLO ACEPTA VALORES NUMERICOS
    } else {
        log_warning(LOGGER_CPU, "Error: Registro invalido en set_registros.");
    }
}

//Lee el valor de memoria correspondiente a
//la Direccion Logica que se encuentra en el Registro Direccion
//y lo almacena en el Registro Datos.
void read_mem(char* reg_datos, char* reg_direccion, int socket) {
    // Obtener los punteros a los registros
    uint32_t *registro_datos_ptr = obtener_registro(reg_datos);
    uint32_t *registro_direccion_ptr = obtener_registro(reg_direccion);

    // Validar que los registros sean validos
    if (registro_datos_ptr == NULL || registro_direccion_ptr == NULL) {
        log_warning(LOGGER_CPU, "Error: Registro invalido en read_mem.")
        return;
    }

    // Obtener la direccion logica del registro de direccion
    uint32_t direccion_logica = *registro_direccion_ptr;

    // Traducir la direccion logica a direccion fisica
    uint32_t direccion_fisica = traducir_direccion(direccion_logica, pcb_actual->pid);

    // Verificar si la traduccion resulto en un Segmentation Fault
    if (direccion_fisica == SEGMENTATION_FAULT) {
        enviar_interrupcion_segfault(pcb_actual->pid, socket);
        return;
    }

    // Leer el valor de memoria desde la direccion fisica
    uint32_t valor_leido = recibir_valor_de_memoria(socket, direccion_fisica);

    // Almacenar el valor en el registro de datos
    *registro_datos_ptr = valor_leido;

    // Loguear la accion de lectura
    log_info(LOGGER_CPU, "PID: %d - Leer Memoria - Direccion Fisica: %d - Valor: %d",
             pcb_actual->pid, direccion_fisica, valor_leido);
}


// void read_mem(char* reg_datos, char* reg_direccion, int socket) {
//     // Paso 1: Obtener los punteros a los registros de datos y direccion
//     uint32_t *registro_datos_ptr = obtener_registro(reg_datos);
//     uint32_t *registro_direccion_ptr = obtener_registro(reg_direccion);

//     // Validacion de los punteros a registros
//     if (registro_datos_ptr == NULL || registro_direccion_ptr == NULL) {
//         fprintf(stderr, "Error: Registro invalido en read_mem.\n");
//         return;
//     }

//     // Paso 2: Obtener la direccion logica del registro de direccion
//     uint32_t direccion_logica = *registro_direccion_ptr;

//     // Paso 3: Traducir la direccion logica a fisica usando la MMU
//     t_list* lista_direcciones_fisicas = traducir_direccion(direccion_logica, pcb_actual->pid);

//     // Validar que la traduccion sea correcta
//     if (lista_direcciones_fisicas == NULL) {
//         fprintf(stderr, "Error: Segmentation Fault en read_mem.\n");
//         enviar_interrupcion_segfault(pcb_actual->pid, socket);
//         return;
//     }

//     // Paso 4: Solicitar a memoria el valor en la direccion fisica
//     void* dato_obtenido = recibir_dato_de_memoria(socket, LOGGER_CPU, lista_direcciones_fisicas, 4); // Leer 4 bytes (32 bits)

//     // Validar si la memoria devolvio un valor correcto
//     if (dato_obtenido == NULL) {
//         fprintf(stderr, "Error: Fallo al recibir el dato desde memoria.\n");
//         list_destroy_and_destroy_elements(lista_direcciones_fisicas, free); // Liberar lista de direcciones fisicas
//         return;
//     }

//     // Paso 5: Almacenar el valor leido en el registro de datos
//     *registro_datos_ptr = *((uint32_t *)dato_obtenido);

//     // Paso 6: Loguear la accion
//     for (int i = 0; i < list_size(lista_direcciones_fisicas); i++) {
//         t_direcciones_fisicas *direccion = list_get(lista_direcciones_fisicas, i);
//         log_info(LOGGER_CPU, "PID: %d - Accion: LEER - Direccion Fisica: %d - Valor: %d",
//                  pcb_actual->pid, direccion->direccion_fisica, *((uint32_t *)dato_obtenido));
//     }

//     // Liberacion de recursos utilizados:
//     free(dato_obtenido);  // Liberar el valor recibido de memoria
//     list_destroy_and_destroy_elements(lista_direcciones_fisicas, free);  // Liberar lista de direcciones fisicas
    
// }



//Lee el valor del Registro Datos y lo escribe en la 
//direccion fisica de memoria obtenida a partir de la 
//Direccion Logica almacenada en el Registro Direccion.
void write_mem(char* reg_direccion, char* reg_datos, int socket) {
    // Obtener los punteros a los registros
    uint32_t *reg_dire = obtener_registro(reg_direccion);
    uint32_t *reg_dat = obtener_registro(reg_datos);

    // Validar que los registros sean validos
    if (registro_direccion_ptr == NULL || registro_datos_ptr == NULL) {
        log_warning(LOGGER_CPU, "Error: Registro invalido en write_mem.")
        return;
    }

    // Obtener la direccion logica del registro de direccion
    uint32_t direccion_logica = *registro_direccion_ptr;

    // Traducir la direccion logica a direccion fisica
    uint32_t direccion_fisica = traducir_direccion(direccion_logica, pcb_actual->pid);
    
    // Verificar si la traduccion resulto en un Segmentation Fault
    if (direccion_fisica == SEGMENTATION_FAULT) {
        enviar_interrupcion_segfault(pcb_actual->pid, socket);
        return;
    }

    // Enviar el valor a la memoria para escribir en la direccion fisica
    enviar_valor_a_memoria(socket, direccion_fisica, *registro_datos_ptr);

    // Loguear la accion de escritura
    log_info(LOGGER_CPU, "PID: %d - Escribir Memoria - Direccion Fisica: %d - Valor: %d",
             pcb_actual->pid, direccion_fisica, *registro_datos_ptr);
}

//Suma al Registro Destino el Registro Origen y deja el resultado en el Registro Destino.
void sum_registros(char* destino, char* origen)
{
    uint32_t *dest = obtener_registro(destino);
    uint32_t *orig = obtener_registro(origen); 

    if(dest && orig) {
        *dest += *orig;
    } else {
        log_warning(LOGGER_CPU, "Error: Registro invalido en sum_registros.")
    }
}

//Resta al Registro Destino el Registro Origen y deja el resultado en el Registro Destino.
void sub_registros(char* destino, char* origen)
{
    uint32_t *dest = obtener_registro(destino);
    uint32_t *orig = obtener_registro(origen); 

    if(dest && orig) {
        *dest -= *orig;
    } else {
        log_warning(LOGGER_CPU, "Error: Registro invalido en sub_registros.")
    }
}

//Si el valor del registro es distinto de cero, actualiza el program counter
//al numero de instruccion pasada por parametro.
void jnz_pc(char* registro, char* instruccion)
{
    uint32_t *reg = obtener_registro(registro);

    if(reg) {
        if(*reg != 0) {
            uint32_t nueva_instruccion = atoi(instruccion);
            pcb_actual->contexto_ejecucion->t_registros->program_counter = nueva_instruccion;
        } else {
            log_warning(LOGGER_CPU, "El registro %s es igual a cero, no se actualiza el IP", registro);
        }
    } else {
        log_warning(LOGGER_CPU, "Error: Registro invalido en jnz_pc.")
    }
}

//Escribe en el archivo de log el valor del registro.
void log_registro(char* registro)
{
    uint32_t reg = obtener_registro(registro);

    if(reg) {
        log_info(LOGGER_CPU, "LOG - Registro %s: %d", registro, *reg);
    } else {
        log_warning(LOGGER_CPU, "Error: Registro invalido en log_registro.")
    }
}

uint32_t* obtener_registro(char* registro){

    if(strcmp(registro,"AX")==0)
        return &(pcb_actual->contexto_ejecucion->registros->AX);
    if(strcmp(registro,"BX")==0)
        return &(pcb_actual->contexto_ejecucion->registros->BX);
    if(strcmp(registro,"CX")==0)
        return &(pcb_actual->contexto_ejecucion->registros->CX);
    if(strcmp(registro,"DX")==0)
        return &(pcb_actual->contexto_ejecucion->registros->DX);
    if(strcmp(registro,"EX")==0)
        return &(pcb_actual->contexto_ejecucion->registros->EX);
    if(strcmp(registro,"GX")==0)
        return &(pcb_actual->contexto_ejecucion->registros->GX);
    if(strcmp(registro,"HX")==0)
        return &(pcb_actual->contexto_ejecucion->registros->HX);
    if(strcmp(registro,"PC")==0) 
        return &(pcb_actual->contexto_ejecucion->registros->PC);
    else return NULL;
}

//FUNCIONES DE ABAJO A REVISAR:
void* recibir_dato_de_memoria(int socket, t_log* logger, t_list* lista_direcciones_fisicas, uint32_t tamanio) {
    // Solicitar a la memoria el valor en las direcciones fisicas (en este caso simplificamos)
    // Enviar la solicitud de lectura a la memoria a traves del socket
    enviar_solicitud_memoria(socket, lista_direcciones_fisicas, tamanio);

    // Reservar memoria para recibir el dato
    void* buffer = malloc(tamanio);
    
    // Recibir el valor de memoria (asumimos que el socket esta preparado para esto)
    if (recv(socket, buffer, tamanio, 0) <= 0) {
        log_error(logger, "Error al recibir datos de memoria.");
        free(buffer);
        return NULL;
    }
    
    // Devolver el buffer con los datos recibidos
    return buffer;
}

void enviar_interrupcion_segfault(uint32_t pid, int socket) {
    // Crear el paquete de interrupcion
    t_paquete* paquete = crear_paquete(SEGF_FAULT);
    
    // Agregar el PID al paquete
    agregar_a_paquete(paquete, &pid, sizeof(uint32_t));
    
    // Enviar el paquete al socket de la memoria o kernel
    enviar_paquete(paquete, socket);
    
    // Liberar el paquete
    eliminar_paquete(paquete);
}