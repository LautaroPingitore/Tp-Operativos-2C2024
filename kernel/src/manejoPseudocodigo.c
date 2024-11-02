#include <include/manejoPseudocodigo.h>

int leer_archivo_pseudocodigo(const char* nombre_archivo, archivo_pseudocodigo archivo) {
    FILE* file = fopen(nombre_archivo, "r");
    if(file == NULL) {
        log_error(LOGGER_KERNEL, "Error al abrir el archivo");
        return -1;
    }

    archivo->cantidad_instrucciones = 0;
    while(fscanf(file, "%s %s %s", archivo->instrucciones[archivo->cantidad_instrucciones].comando,
                 archivo->instrucciones[archivo->cantidad_instrucciones].argumento1
                 archivo->instrucciones[archivo->cantidad_instrucciones].argumento2) == 3) {
        archivo->cantidad_instrucciones++;
    }
    fclose(file);
    return 0;
}

int obtener_recursos_del_proceso(const archivo_pseudocodigo archivo, lista_recursoslista_recursos) {
    lista_recursos->cantidad_recursos = 0;

    
}
