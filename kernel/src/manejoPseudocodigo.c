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

        log_warning(LOGGER_KERNEL, "SE AGREGO INSTRUCCION");

        list_add(archivo->instrucciones, inst);
    }
    
    fclose(file);
    return archivo;
}

void asignar_mutex_a_proceso(t_pcb* pcb, char* nombre_recurso) {
    // VERIFICA SI EL RECURSO YA ESTA ASIGNADO AL PROCESO
    for (int i = 0; i < list_size(pcb->MUTEXS); i++) {
        if (strcmp(nombre_recurso, list_get(pcb->MUTEXS,i)) == 0) {
            log_warning(LOGGER_KERNEL, "El recurso %s ya está asignado al proceso %d", nombre_recurso, pcb->PID);
            return;
        }
    }

    char* nuevo_recurso = strdup(nombre_recurso);
    list_add(pcb->MUTEXS, nuevo_recurso);
}

// MODIFICAR LA FUNCION DE CREAR_PCB
void obtener_recursos_del_proceso(archivo_pseudocodigo* archivo, t_pcb* pcb) {

    for(int i=0; i < list_size(archivo->instrucciones); i++) {
        t_instruccion* inst = list_get(archivo->instrucciones, i);

        if(strcmp(inst->nombre, "MUTEX_CREATE") == 0) {
            bool existe_en_pcb = false; // VERIFICA SI EL RECURSO YA ESTA EN LA LISTA
            for (int i=0; i < list_size(pcb->MUTEXS); i++) {
                if(strcmp(inst->parametro1, list_get(pcb->MUTEXS, i)) == 0) {
                    existe_en_pcb = true;
                    break;
                }
            }

            if (!existe_en_pcb) {  
                asignar_mutex_a_proceso(pcb, inst->parametro1);

                // VERIFICA SI EL RERSO YA SE INICIALIZO POR OTRO PROCESO
                bool existe_en_globales = false;
                for(int j=0; j < list_size(recursos_globales.recursos); j++) {
                    t_recurso* actual = list_get(recursos_globales.recursos,j);
                    if(strcmp(inst->parametro1, actual->nombre_recurso) == 0) {
                        existe_en_globales = true;
                        break;
                    }
                }
                if(!existe_en_globales) {
                    t_recurso* recurso_nuevo = malloc(sizeof(t_recurso));
                    strcpy(recurso_nuevo->nombre_recurso, inst->parametro1);
                    pthread_mutex_init(&recurso_nuevo->mutex, NULL);
                    list_add(recursos_globales.recursos, recurso_nuevo); // USAR UNA VARIABLE GLOBAL PARA MANEJAR LA CANTIDAD DE RECURSOS TOTALES
                }
            }
        }
    }
}

// HACER ESTA FUNCION
void procesar_archivo(archivo_pseudocodigo archivo) {
    // OBTIENE LA INSTRUCCION Y LA EJECUTA USANDO LA FUNCION EXCECUTE DE CPU O USANDO CICLO INSTRUCCION
}