#include <include/manejoPseudocodigo.h>

int leer_archivo_pseudocodigo(char* nombre_archivo, archivo_pseudocodigo archivo) {
    FILE* file = fopen(nombre_archivo, "r");
    if(file == NULL) {
        log_error(LOGGER_KERNEL, "Error al abrir el archivo");
        return -1;
    }

    archivo->cantidad_instrucciones = 0;

    char linea[100];
    while(fgets(linea, sizeof(linea), file) != NULL) {
        instruccion *inst = &archivo->instrucciones[archivo->cantidad_instrucciones];
        // SSCANF, TE DICE CUANTOS VALORES PUDO METER
        int elementos = sscanf(linea, "%s %s %d", inst->comando, inst->argumento1, inst->argumento2);

        if (elementos == 2) {
            inst->argumento2 = -1;
        } else if (elementos == 1) {
            strcpy(inst->argumento1, "");
            inst->argumento2 = -1;
        }

        archivo->cantidad_instrucciones++;
    }
    
    fclose(file);
    return 0;
}

void asignar_mutex_a_proceso(t_pcb* pcb, char* nombre_recurso) {
    int cantidad_recursos = pcb->CANTIDAD_RECURSOS;
    strcpy(pcb->MUTEXS[cantidad_recursos], nombre_recurso)
    pcb->CANTIDAD_RECURSOS++;
}

// MODIFICAR LA FUNCION DE CREAR_PCB
void obtener_recursos_del_proceso(archivo_pseudocodigo archivo, lista_recursos lista_recursos, t_pcb* pcb) {
    lista_recursos->cantidad_recursos = 0;

    for(int i=0; i < archivo->cantidad_instrucciones; i++) {
        instruccion inst = archivo->instrucciones[i]

        if(strcmp(inst.comando, "MUTEX_CREATE") == 0) {
            bool existe = false; // VERIFICA SI EL RECURSO YA ESTA EN LA LISTA
            for (int i=0; i < pcb->cantidad_recursos; i++) {
                if(strcmp(inst.argumento1, pcb->MUTEXS[i]) == 0) {
                    exite = true;
                    break
                }
            }

            if (!existe) {  
                asignar_mutex_a_proceso(pcb, inst.argumento1, mutex);
            }
        }
    }
}






