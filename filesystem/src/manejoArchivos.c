#include "include/gestor.h"

char* bitmap_memoria = NULL;

void cargar_bitmap() {
    char bitmap_path[256];
    sprintf(bitmap_path, "%s/bitmap.dat", MOUNT_DIR);

    FILE* bitmap = fopen(bitmap_path, "r");
    if (!bitmap) {
        log_error(LOGGER_FILESYSTEM, "Error al abrir bitmap.dat");
        exit(EXIT_FAILURE);
    }

    // Reservar memoria para el bitmap
    size_t bitmap_size = (BLOCK_COUNT + 7) / 8 ;
    bitmap_memoria = malloc(bitmap_size);// bitmap_memoria de donde sale??? Y pq la cuenta de +7 / 8 ???
    if (!bitmap_memoria) {
        log_error(LOGGER_FILESYSTEM, "Error al asignar memoria para el bitmap.");
        fclose(bitmap);
        exit(EXIT_FAILURE);
    }

    // Leer el contenido del archivo en la memoria
    fread(bitmap_memoria, 1, bitmap_size, bitmap);
    fclose(bitmap);
}

void guardar_bitmap() {
    if(!bitmap_memoria) {
        log_error(LOGGER_FILESYSTEM, "No se puede guardar: el bitmap no está cargado en memoria");
        return;
    }

    char bitmap_path[256];
    sprintf(bitmap_path, "%s/bitmap.dat", MOUNT_DIR);

    FILE* bitmap = fopen(bitmap_path, "w+");
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

    FILE* metadata = fopen(metadata_path, "w+");
    if (!metadata) {
        log_error(LOGGER_FILESYSTEM, "Error al crear archivo de metadata: %s", metadata_path);
        liberar_bloque(bloque_indice);
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
    if (!bloques_file) {
        log_error(LOGGER_FILESYSTEM, "Error al abrir bloques.dat");
        liberar_bloque(bloque_indice);
        return;
    }

    // CALCULO DE CUANTOS BLOQUES SE NECESITAN
    int cantidad_bloques = (tamanio + BLOCK_SIZE - 1) / BLOCK_SIZE;

    for(int i=0; i<cantidad_bloques; i++) {
        int bloque_datos = asignar_bloque();
        if (bloque_datos == -1) {
            log_error(LOGGER_FILESYSTEM, "Error al asignar bloque de datos.");
            liberar_bloque(bloque_indice);
            fclose(bloques_file);
            return;
        }

        // SIMULACION DEL RETARDO DE ACCESO
        usleep(RETARDO_ACCESO_BLOQUE * 1000);

        // ESCRIBE LOS DATOS EN EL BLOQUE
        fseek(bloques_file, bloque_datos * BLOCK_SIZE, SEEK_SET);
        size_t bytes_a_escribir = (i == cantidad_bloques - 1) ? (tamanio % BLOCK_SIZE) : BLOCK_SIZE;
        fwrite(contenido + i * BLOCK_SIZE, 1, bytes_a_escribir, bloques_file);

        log_info(LOGGER_FILESYSTEM, "## Bloque asignado: %d - Archivo: %s - Bloques Libres: %d", bloque_datos, bloques_path, obtener_bloques_libres());
        log_info(LOGGER_FILESYSTEM, "## Acceso Bloque - Archivo: %s - Tipo Bloque: DATOS - Bloque File System: %d", bloques_path, bloque_datos);
    }

    fclose(bloques_file);
}

// GESTION Y ASIGNACION DE BLOQUES EN EL ARCHIVO bitmap.dat
int asignar_bloque() {
    if (!bitmap_memoria) {
        log_error(LOGGER_FILESYSTEM, "Bitmap no está cargado en memoria.");
        return -1;
    }
    // size_t bitmap_size = (BLOCK_COUNT + 7) / 8;

    // LEE EL BITMAP Y BUSCA EL PRIMER BLOQUE LIBRE
    for (int i = 0; i < BLOCK_COUNT; i++) {
        unsigned char byte = bitmap_memoria[i / 8];
        if (!(byte & (1 << (i % 8)))) {
            bitmap_memoria[i / 8] |= (1 << (i % 8));
            return i;
        }
    }

    log_error(LOGGER_FILESYSTEM, "No hay bloques libres disponibles.");
    return -1; // NO SE ENCONTRO NINGUN BLOQUE LIBRE
}

int hay_espacion_suficiente(int tamanio_archivo) {
    int bloques_necesarios = (tamanio_archivo + BLOCK_SIZE - 1) / BLOCK_SIZE;
    return (obtener_bloques_libres() >= bloques_necesarios);

}

int obtener_bloques_libres() {
    if (!bitmap_memoria) {
        log_error(LOGGER_FILESYSTEM, "Bitmap no está cargado en memoria.");
        return -1;
    }

    int bloques_libres = 0;

    // Iteramos sobre cada bloque en el bitmap
    for (int i = 0; i < BLOCK_COUNT; i++) {
        if (!(bitmap_memoria[i / 8] & (1 << (i % 8)))) {
            bloques_libres++;
        }
    }

    return bloques_libres;
}

// LIBERAR BLOQUES (OPCIONAL)
void liberar_bloque(int numero_bloque) {
    if (!bitmap_memoria || numero_bloque < 0 || numero_bloque >= BLOCK_COUNT) {
        log_error(LOGGER_FILESYSTEM, "Bloque inválido o bitmap no cargado en memoria.");
        return;
    }

    bitmap_memoria[numero_bloque / 8] &= ~(1 << (numero_bloque % 8));
}