#include "include/manejoPseudocodigo.h"

archivo_pseudocodigo* leer_archivo_pseudocodigo(char* path_proceso) {
    archivo_pseudocodigo* archivo = NULL;

    FILE* file = fopen(path_proceso, "r");
    if(file == NULL) {
        log_error(LOGGER_KERNEL, "Error al abrir el archivo");
        return NULL;
    }

    archivo->path_archivo = path_proceso;
    archivo->cantidad_instrucciones = 0;

    char linea[100];
    while(fgets(linea, sizeof(linea), file) != NULL) {
        t_instruccion *inst = &archivo->intrucciones[archivo->cantidad_instrucciones]; //Que onda esta linea?
        // SSCANF, TE DICE CUANTOS VALORES PUDO METER
        int elementos = sscanf(linea, "%d %s %d %d", inst->nombre, inst->parametro1, inst->parametro2, inst->parametro3);

        if (elementos == 3) {
            inst->parametro2 = -1;
        } else if (elementos == 2) {
            inst->parametro2 = -1;
            inst->parametro3 = -1;
        } else if (elementos == 1) {
            strcpy(inst->parametro1, "");
            inst->parametro2 = -1;
            inst->parametro3 = -1;
        }

        archivo->cantidad_instrucciones++;
    }
    
    fclose(file);
    return archivo;
}

void asignar_mutex_a_proceso(t_pcb* pcb, char* nombre_recurso) {
    int cantidad_recursos = pcb->CANTIDAD_RECURSOS;
    strcpy(pcb->MUTEXS[cantidad_recursos].nombre_recurso, nombre_recurso);
    pthread_mutex_init(&pcb->MUTEXS[cantidad_recursos].mutex, NULL);
    pcb->CANTIDAD_RECURSOS++;
}

// MODIFICAR LA FUNCION DE CREAR_PCB
void obtener_recursos_del_proceso(archivo_pseudocodigo archivo, char** recursos_globales, t_pcb* pcb) {

    for(int i=0; i < archivo.cantidad_instrucciones; i++) {
        t_instruccion inst = archivo.intrucciones[i];

        if(strcmp(inst.nombre, "MUTEX_CREATE") == 0) {
            bool existe = false; // VERIFICA SI EL RECURSO YA ESTA EN LA LISTA
            for (int i=0; i < pcb->CANTIDAD_RECURSOS; i++) {
                if(strcmp(inst.parametro1, pcb->MUTEXS[i].nombre_recurso) == 0) {
                    existe = true;
                    break;
                }
            }

            if (!existe) {  
                asignar_mutex_a_proceso(pcb, inst.parametro1);
                recursos_globales[0] = inst.parametro1; // USAR UNA VARIABLE GLOBAL PARA MANEJAR LA CANTIDAD DE RECURSOS TOTALES
            }
        }
    }
}

void procesar_archivo(archivo_pseudocodigo archivo) {
    // OBTIENE LA INSTRUCCION Y LA EJECUTA USANDO LA FUNCION EXCECUTE DE CPU O USANDO CICLO INSTRUCCION
}