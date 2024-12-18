#include "include/gestor.h"

archivo_pseudocodigo* leer_archivo_pseudocodigo(char* path_proceso) {

    if (path_proceso == NULL) {
        log_error(LOGGER_KERNEL, "Ruta del archivo de pseudocódigo no proporcionada");
        return NULL;
    }
    FILE* file = fopen(path_proceso, "r");
    if(file == NULL) {
        log_error(LOGGER_KERNEL, "Error al abrir el archivo");
        return NULL;
    }

    archivo_pseudocodigo* archivo = malloc(sizeof(archivo_pseudocodigo));
    archivo->path_archivo = strdup(path_proceso);
    archivo->instrucciones = list_create();

    char linea[100];
    while(fgets(linea, sizeof(linea), file) != NULL) {
        // REMUEVE EL SALTO DE LINEA SI HAY
        linea[strcspn(linea, "\n")] = 0;
        t_instruccion* inst = malloc(sizeof(t_instruccion));
        
        inst->nombre = malloc(32);       
        inst->parametro1 = malloc(32);
        inst->parametro2 = malloc(32);

        // SSCANF, TE DICE CUANTOS VALORES PUDO METER
        int elementos = sscanf(linea, "%s %s %s %d", inst->nombre, inst->parametro1, inst->parametro2, &inst->parametro3);

        if (elementos == 1) {
            strcpy(inst->parametro1, "");
            strcpy(inst->parametro2, "");
            inst->parametro3 = -1;
        } else if (elementos == 2) {
            strcpy(inst->parametro2, "");
            inst->parametro3 = -1;
        } else if (elementos == 3) {
            inst->parametro3 = -1;
        }

        list_add(archivo->instrucciones, inst);
    }
    
    fclose(file);
    return archivo;
}

void asignar_mutex_a_proceso(t_pcb* pcb, char* nombre_recurso) {
    char* nuevo_recurso = strdup(nombre_recurso);
    list_add(pcb->MUTEXS, nuevo_recurso);
}