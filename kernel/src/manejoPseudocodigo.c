#include "include/gestor.h"

archivo_pseudocodigo* leer_archivo_pseudocodigo(char* path_proceso) {
    archivo_pseudocodigo* archivo = NULL;

    FILE* file = fopen(path_proceso, "r");
    if(file == NULL) {
        log_error(LOGGER_KERNEL, "Error al abrir el archivo");
        return NULL;
    }

    archivo->path_archivo = path_proceso;
    archivo->instrucciones = list_create();

    char linea[100];
    while(fgets(linea, sizeof(linea), file) != NULL) {
        t_instruccion *inst = malloc(sizeof(t_instruccion));
        // SSCANF, TE DICE CUANTOS VALORES PUDO METER
        int elementos = sscanf(linea, "%s %s %s %d", inst->nombre, inst->parametro1, inst->parametro2, &inst->parametro3);

        if (elementos == 3) {
            inst->parametro3 = -1;
        } else if (elementos == 2) {
            strcpy(inst->parametro2, "");
            inst->parametro3 = -1;
        } else if (elementos == 1) {
            strcpy(inst->parametro1, "");
            strcpy(inst->parametro2, "");
            inst->parametro3 = -1;
        }

        list_add(archivo->instrucciones, inst);
    }
    
    fclose(file);
    return archivo;
}

void asignar_mutex_a_proceso(t_pcb* pcb, char* nombre_recurso) {
    int cantidad_recursos = pcb->CANTIDAD_RECURSOS;
    strcpy(pcb->MUTEXS[cantidad_recursos], nombre_recurso);
    pcb->CANTIDAD_RECURSOS++;
}

// MODIFICAR LA FUNCION DE CREAR_PCB
void obtener_recursos_del_proceso(archivo_pseudocodigo* archivo, t_pcb* pcb) {

    for(int i=0; i < list_size(archivo->instrucciones); i++) {
        t_instruccion* inst = list_get(archivo->instrucciones, i);

        if(strcmp(inst->nombre, "MUTEX_CREATE") == 0) {
            bool existe = false; // VERIFICA SI EL RECURSO YA ESTA EN LA LISTA
            for (int i=0; i < pcb->CANTIDAD_RECURSOS; i++) {
                if(strcmp(inst->parametro1, pcb->MUTEXS[i]) == 0) {
                    existe = true;
                    break;
                }
            }

            if (!existe) {  
                asignar_mutex_a_proceso(pcb, inst->parametro1);
                t_recurso* recurso_nuevo = malloc(sizeof(t_recurso));
                strcpy(recurso_nuevo->nombre_recurso, pcb->MUTEXS[i]);
                list_add(recursos_globales.recursos, recurso_nuevo);// USAR UNA VARIABLE GLOBAL PARA MANEJAR LA CANTIDAD DE RECURSOS TOTALES
                pthread_mutex_init(list_get(recursos_globales.recursos, recursos_globales.cantidad_recursos), NULL);
                recursos_globales.cantidad_recursos ++;
            }
        }
    }
}

void procesar_archivo(archivo_pseudocodigo archivo) {
    // OBTIENE LA INSTRUCCION Y LA EJECUTA USANDO LA FUNCION EXCECUTE DE CPU O USANDO CICLO INSTRUCCION
}