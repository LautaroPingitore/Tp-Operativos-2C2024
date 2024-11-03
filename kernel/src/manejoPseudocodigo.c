#include <include/manejoPseudocodigo.h>

archivo_pseudocodigo* leer_archivo_pseudocodigo(char* path_proceso) {
    archivo_pseudocodigo* archivo = NULL;

    FILE* file = fopen(path_proceso, "r");
    if(file == NULL) {
        log_error(LOGGER_KERNEL, "Error al abrir el archivo");
        return -1;
    }

    archivo->path_archivo = path_proceso;
    archivo->cantidad_instrucciones = 0;

    char linea[100];
    while(fgets(linea, sizeof(linea), file) != NULL) {
        t_instruccion *inst = &archivo->instrucciones[archivo->cantidad_instrucciones]; //Que onda esta linea?
        // SSCANF, TE DICE CUANTOS VALORES PUDO METER
        int elementos = sscanf(linea, "%s %s %d %d", inst->comando, inst->parametro1, inst->parametro2, inst->parametro3);

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
    strcpy(pcb->MUTEXS[cantidad_recursos].nombre_recurso, nombre_recurso, sizeof(pcb->MUTEXS[cantidad_recursos].nombre_recurso) -1 );
    pcb->MUTEXS[cantidad_recursos].mutex = NULL;
    pcb->CANTIDAD_RECURSOS++;
}

// MODIFICAR LA FUNCION DE CREAR_PCB
void obtener_recursos_del_proceso(archivo_pseudocodigo archivo, lista_recursos recursos_globales, t_pcb* pcb) {
    recursos_globales->cantidad_recursos = 0;

    for(int i=0; i < archivo->cantidad_instrucciones; i++) {
        instruccion inst = archivo->instrucciones[i]

        if(strcmp(inst.comando, "MUTEX_CREATE") == 0) {
            bool existe = false; // VERIFICA SI EL RECURSO YA ESTA EN LA LISTA
            for (int i=0; i < pcb->cantidad_recursos; i++) {
                if(strcmp(inst.parametro1, pcb->MUTEXS[i]) == 0) {
                    exite = true;
                    break
                }
            }

            if (!existe) {  
                asignar_mutex_a_proceso(pcb, inst.parametro1);
                recursos_globales->recursos[cantidad_recursos].nombre_recurso = inst.parametro1;
                recursos_globales->recursos[cantidad_recursos].mutex = NULL;
                recursos_globales->cantidad_recursos ++ ;
            }
        }
    }
}

void procesar_archivo(archivo_pseudocodigo* archivo) {
    // OBTIENE LA INSTRUCCION Y LA EJECUTA USANDO LA FUNCION EXCECUTE DE CPU O USANDO CICLO INSTRUCCION
}