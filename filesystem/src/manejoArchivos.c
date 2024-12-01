#include "include/gestor.h"

void cargar_bitmap() {
    char bitmap_path[256];
    sprintf(bitmap_path, "%s/bitmap.dat", MOUNT_DIR);

    FILE* bitmap = fopen(bitmap_path, "r");
    if (!bitmap) {
        log_error(LOGGER_FILESYSTEM, "Error al abrir bitmap.dat");
        exit(EXIT_FAILURE);
    }

    // Reservar memoria para el bitmap
    bitmap_memoria = malloc((BLOCK_COUNT + 7) / 8);// bitmap_memoria de donde sale??? Y pq la cuenta de +7 / 8 ???
    if (!bitmap_memoria) {
        log_error(LOGGER_FILESYSTEM, "Error al asignar memoria para el bitmap.");
        fclose(bitmap);
        exit(EXIT_FAILURE);
    }

    // Leer el contenido del archivo en la memoria
    fread(bitmap_memoria, 1, (BLOCK_COUNT + 7) / 8, bitmap);
    fclose(bitmap);

    log_info(LOGGER_FILESYSTEM, "Bitmap cargado desde %s", bitmap_path);
}

void guardar_bitmap() {
    char bitmap_path[256];
    sprintf(bitmap_path, "%s/bitmap.dat", MOUNT_DIR);

    FILE* bitmap = fopen(bitmap_path, "w");
    if (!bitmap) {
        log_error(LOGGER_FILESYSTEM, "Error al guardar bitmap.dat");
        return;
    }

    fwrite(bitmap_memoria, 1, (BLOCK_COUNT + 7) / 8, bitmap);
    fclose(bitmap);

    log_info(LOGGER_FILESYSTEM, "Bitmap guardado correctamente en %s", bitmap_path);
}


int crear_archivo_dump(char* nombre_archivo, char* contenido, int tamanio) {
    // VERIFICAR SI HAY BLOQUES DISPONIBLES
    if(!hay_espacion_suficiente(tamanio)) {
        log_error(LOGGER_FILESYSTEM, "No hay espacio suficiente para el archivo: %s", nombre_archivo);
        return -1;
    }

    // ASIGNAR LOS BLOQUES DE INDICE Y LOS BLOQUES DE DATOS EN EL BITMAP
    int bloque_indice = asignar_bloque();
    if(bloque_indice == -1) {
        log_error(LOGGER_FILESYSTEM, "Error al asignar bloque de índice");
        return -1;
    }

    // CREAR ARCHIVO DE METADATA (NOMBRE FORMATEADO)
    char metadata_path[256];
    sprintf(metadata_path, "%s/files/%s.dmp", MOUNT_DIR, nombre_archivo);
    FILE* metadata = fopen(metadata_path, "w");
    if (!metadata) {
        log_error(LOGGER_FILESYSTEM, "Error al crear archivo de metadata: %s", metadata_path);
        return -1;
    }

    // ESCRIBIR LA INFORMACION EN EL ARCHIVO DE METADATA
    fprintf(metadata, "SIZE=%d\nINDEX_BLOCK=%d\n", tamanio, bloque_indice);
    fclose(metadata);

    // ESCRIBIR EL CONTENIDO EN LOS BLOQUES DE DATOS
    escribir_en_bloques(contenido, tamanio, bloque_indice);

    log_info(LOGGER_FILESYSTEM, "Archivo Creado: %s - Tamaño: %d", nombre_archivo, tamanio);
    return 0;
}

// FUNCION LA CUAL TOMA Y ESCRIBE LOS DATOS EN LOS BLOQUES DE bloques.dat
void escribir_en_bloques(char* contenido, int tamanio, int bloque_indice) {
    // ABRE BLOQUES.DAT PARA ESCRIBIR
    char bloques_path[256];
    sprintf(bloques_path, "%s/bloques.dat", MOUNT_DIR);
    FILE* bloques_file = fopen(bloques_path, "r+");

    // CALCULO DE CUANTOS BLOQUES SE NECESITAN
    int cantidad_bloques = (tamanio + BLOCK_SIZE - 1) / BLOCK_SIZE;

    for(int i=0; i<cantidad_bloques; i++) {
        int bloque_datos = asignar_bloque();

        // SIMULACION DEL RETARDO DE ACCESO
        usleep(RETARDO_ACCESO_BLOQUE * 1000);

        // ESCRIBE LOS DATOS EN EL BLOQUE
        fseek(bloques_file, bloque_datos * BLOCK_SIZE, SEEK_SET);
        fwrite(contenido + i * BLOCK_SIZE, 1, BLOCK_SIZE, bloques_file);

        log_info(LOGGER_FILESYSTEM, "## Bloque asignado: %d - Archivo: %s - Bloques Libres: %d", bloque_datos, bloques_path, obtener_bloques_libres());
        log_info(LOGGER_FILESYSTEM, "## Acceso Bloque - Archivo: %s - Tipo Bloque: DATOS - Bloque File System: %d", bloques_path, bloque_datos);
    }

    fclose(bloques_file);
}

// GESTION Y ASIGNACION DE BLOQUES EN EL ARCHIVO bitmap.dat
int asignar_bloque() {
    char bitmap_path[256];
    sprintf(bitmap_path, "%s/bitmap.dat", MOUNT_DIR);
    FILE* bitmap = fopen(bitmap_path, "r+");

    if (!bitmap) {
        log_error(LOGGER_FILESYSTEM, "Error al abrir bitmap.dat");
        return -1;
    }

    // LEE EL BITMAP Y BUSCA EL PRIMER BLOQUE LIBRE
    for(int i=0; i<BLOCK_COUNT; i++) {
        fseek(bitmap, i/8, SEEK_SET);
        unsigned char byte;
        fread(&byte, 1, 1, bitmap);

        if (!(byte & (1 << (i % 8)))) {  // VERIFICA SI EL BIT ESTA LIBRE = 0
            // LO MARCA COMO OCUPADO
            byte |= (1 << (i % 8));
            fseek(bitmap, i / 8, SEEK_SET);
            fwrite(&byte, 1, 1, bitmap);

            fclose(bitmap);
            return i;  // RETORNA EL NRO DEL BLOQUE ASIGNADO
        }
    }

    fclose(bitmap);
    return -1; // NO SE ENCONTRO NINGUN BLOQUE LIBRE
}

int hay_espacion_suficiente(int tamanio_archivo) {
    int bloques_necesarios = (tamanio_archivo + BLOCK_SIZE - 1) / BLOCK_SIZE;
    int bloques_libres = 0;

    char bitmap_path[256];
    sprintf(bitmap_path, "%s/bitmap.dat", MOUNT_DIR);
    FILE* bitmap = fopen(bitmap_path, "r");
    if (!bitmap) {
        log_error(LOGGER_FILESYSTEM, "Error al abrir bitmap.dat");
        return 0;  // No hay espacio si no se puede abrir el bitmap
    }

    for(int i=0; i<BLOCK_COUNT; i++) {
        fseek(bitmap, i/8, SEEK_SET); // NOS POSICIONAMOS EN EL BYTE CORRESPONDIENTE
        unsigned char byte;
        fread(&byte, 1, 1, bitmap);

        if (!(byte & (1 << (i % 8)))) {  // SI EL BIT ESTA EN O ==> ESTA LIBRE
            bloques_libres++;
            if (bloques_libres >= bloques_necesarios) {
                fclose(bitmap);
                return 1;  // HAY ESPACIO SUFICIENTE
            }
        }
    }

    fclose(bitmap);
    return 0; // NO HAY ESPACIO SUFICIENTE

}

int obtener_bloques_libres() {
    int bloques_libres = 0;
    char bitmap_path[256];
    sprintf(bitmap_path, "%s/bitmap.dat", MOUNT_DIR);
    FILE* bitmap = fopen(bitmap_path, "r");

    if (!bitmap) {
        log_error(LOGGER_FILESYSTEM, "Error al abrir bitmap.dat");
        return -1;
    }

    // Iteramos sobre cada bloque en el bitmap
    for (int i = 0; i < BLOCK_COUNT; i++) {
        fseek(bitmap, i / 8, SEEK_SET);
        unsigned char byte;
        fread(&byte, 1, 1, bitmap);

        // Verificamos si el bit correspondiente al bloque está libre
        if (!(byte & (1 << (i % 8)))) {  // Si el bit está en 0, el bloque está libre
            bloques_libres++;
        }
    }

    fclose(bitmap);
    return bloques_libres;
}

// LIBERAR BLOQUES (OPCIONAL)
void liberar_bloque(int numero_bloque) {
    char bitmap_path[256];
    sprintf(bitmap_path, "%s/bitmap.dat", MOUNT_DIR);
    FILE* bitmap = fopen(bitmap_path, "r+");
    if (!bitmap) {
        log_error(LOGGER_FILESYSTEM, "Error al abrir bitmap.dat");
        return;
    }

    fseek(bitmap, numero_bloque / 8, SEEK_SET);
    unsigned char byte;
    fread(&byte, 1, 1, bitmap);

    byte &= ~(1 << (numero_bloque % 8));  // Marcar el bloque como libre (poner el bit en 0)
    fseek(bitmap, numero_bloque / 8, SEEK_SET);
    fwrite(&byte, 1, 1, bitmap);

    fclose(bitmap);
}