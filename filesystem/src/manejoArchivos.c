#include "include/gestor.h"

// git clone https://github.com/sisoputnfrba/so-deploy.git
// cd so-deploy
// ./deploy.sh -r=release -p=utils -p=kernel -p=cpu -p=memoria -p=filesystem "tp-2024-2c-Los-Galacticos-SO"
// https://github.com/sisoputnfrba/the-last-of-c-pruebas.git

char* bitmap_memoria = NULL;

void cargar_bitmap() {
    char bitmap_path[256];
    //snprintf(bitmap_path, sizeof(bitmap_path),"%s/bitmap.dat", MOUNT_DIR);
    sprintf(bitmap_path, "%s/bitmap.dat", MOUNT_DIR);

    FILE* bitmap = fopen(bitmap_path, "r");
    if (!bitmap) {
        log_error(LOGGER_FILESYSTEM, "Error al abrir bitmap.dat");
        exit(EXIT_FAILURE);
    }

    // Verificar que el archivo tiene el tamaño esperado
    fseek(bitmap, 0, SEEK_END);
    size_t file_size = ftell(bitmap);
    rewind(bitmap);

    size_t expected_size = (BLOCK_COUNT + 7) / 8; // SE USA EL + 7 PARA REDONDEAR PARA ARRIBA
    if (file_size != expected_size) {
        log_error(LOGGER_FILESYSTEM, "El tamaño de bitmap.dat (%zu bytes) no coincide con el esperado (%zu bytes)", file_size, expected_size);
        fclose(bitmap);
        exit(EXIT_FAILURE);
    }


    // Reservar memoria para el bitmap
    bitmap_memoria = malloc(expected_size);
    if (!bitmap_memoria) {
        log_error(LOGGER_FILESYSTEM, "Error al asignar memoria para el bitmap.");
        fclose(bitmap);
        exit(EXIT_FAILURE);
    }

    // Leer el contenido del archivo en memoria
    size_t valor = fread(bitmap_memoria, 1, expected_size, bitmap);
    if (valor != expected_size) {
        log_error(LOGGER_FILESYSTEM, "Error al leer bitmap.dat completo. Bytes leídos: %zu", valor);
        free(bitmap_memoria);
        fclose(bitmap);
        exit(EXIT_FAILURE);
    }

    fclose(bitmap);
    log_info(LOGGER_FILESYSTEM, "Bitmap cargado correctamente desde %s", bitmap_path);
}

void guardar_bitmap() {
    if(!bitmap_memoria) {
        log_error(LOGGER_FILESYSTEM, "No se puede guardar: el bitmap no está cargado en memoria");
        return;
    }

    char bitmap_path[256];
    sprintf(bitmap_path, "%s/bitmap.dat", MOUNT_DIR);
    //snprintf(bitmap_path,sizeof(bitmap_path), "%s/bitmap.dat", MOUNT_DIR);

    FILE* bitmap = fopen(bitmap_path, "w+");
    if (!bitmap) {
        log_error(LOGGER_FILESYSTEM, "Error al guardar bitmap.dat");
        return;
    }

    size_t bitmap_size = (BLOCK_COUNT + 7) / 8;
    size_t bytes_escritos = fwrite(bitmap_memoria, 1, bitmap_size, bitmap);
    if (bytes_escritos != bitmap_size) {
        log_error(LOGGER_FILESYSTEM, "Error al escribir el bitmap completo. Bytes escritos: %zu", bytes_escritos);
    } else {
        log_info(LOGGER_FILESYSTEM, "Bitmap guardado correctamente en %s", bitmap_path);
    }

    fflush(bitmap);
    fclose(bitmap);
}

// int crear_archivo_dump(char* nombre_archivo, char* contenido, int tamanio) {
//     // VERIFICAR SI HAY BLOQUES DISPONIBLES
//     if(!hay_espacion_suficiente(tamanio)) {
//         log_error(LOGGER_FILESYSTEM, "No hay espacio suficiente para el archivo: %s", nombre_archivo);
//         return -1;
//     }

//     // ASIGNAR LOS BLOQUES DE INDICE Y LOS BLOQUES DE DATOS EN EL BITMAP
//     int bloque_indice = asignar_bloque();
//     if(bloque_indice == -1) {
//         log_error(LOGGER_FILESYSTEM, "Error al asignar bloque de índice");
//         return -1;
//     }

//     // CREAR ARCHIVO DE METADATA (NOMBRE FORMATEADO)
//     char files_path[256];
//     snprintf(files_path,sizeof(files_path), "%s/files", MOUNT_DIR);
//     if(access(files_path, F_OK) != 0) {
//         if(mkdir(files_path, 0755) == -1) {
//             log_error(LOGGER_FILESYSTEM, "ERROR AL CREAR EL DIRECTORIO FILES");
//         } else {
//             log_info(LOGGER_FILESYSTEM, "Directorio files creado con exito");
//         }
//     }

//     char metadata_path[256];
//     snprintf(metadata_path, sizeof(metadata_path), "%s/%s.dmp", files_path, nombre_archivo);

//     FILE* metadata = fopen(metadata_path, "w+");
//     if (!metadata) {
//         log_error(LOGGER_FILESYSTEM, "Error al crear archivo de metadata: %s", metadata_path);
//         liberar_bloque(bloque_indice);
//         return -1;
//     }

//     // ESCRIBIR LA INFORMACION EN EL ARCHIVO DE METADATA
//     fprintf(metadata, "SIZE=%d\nINDEX_BLOCK=%d\n", tamanio, bloque_indice);
//     fclose(metadata);

//     // ESCRIBIR EL CONTENIDO EN LOS BLOQUES DE DATOS
//     escribir_en_bloques(contenido, tamanio, bloque_indice);

//     log_info(LOGGER_FILESYSTEM, "Archivo Creado: %s - Tamaño: %d", nombre_archivo, tamanio);
//     return 1;
// }

// // FUNCION LA CUAL TOMA Y ESCRIBE LOS DATOS EN LOS BLOQUES DE bloques.dat
// void escribir_en_bloques(char* contenido, int tamanio, int bloque_indice) {
//     // ABRE BLOQUES.DAT PARA ESCRIBIR
//     char bloques_path[256];
//     snprintf(bloques_path, sizeof(bloques_path),"%s/bloques.dat", MOUNT_DIR);
//     FILE* bloques_file = fopen(bloques_path, "r+");
//     if (!bloques_file) {
//         log_error(LOGGER_FILESYSTEM, "Error al abrir bloques.dat");
//         liberar_bloque(bloque_indice);
//         return;
//     }

//     // CALCULO DE CUANTOS BLOQUES SE NECESITAN
//     int cantidad_bloques = (tamanio + BLOCK_SIZE - 1) / BLOCK_SIZE;

//     for(int i=0; i<cantidad_bloques; i++) {
//         int bloque_datos = asignar_bloque();
//         if (bloque_datos == -1) {
//             log_error(LOGGER_FILESYSTEM, "Error al asignar bloque de datos.");
//             liberar_bloque(bloque_indice);
//             fclose(bloques_file);
//             return;
//         }

//         // SIMULACION DEL RETARDO DE ACCESO
//         usleep(RETARDO_ACCESO_BLOQUE * 1000);

//         // ESCRIBE LOS DATOS EN EL BLOQUE
//         fseek(bloques_file, bloque_datos * BLOCK_SIZE, SEEK_SET);
//         size_t bytes_a_escribir = (i == cantidad_bloques - 1) ? (tamanio % BLOCK_SIZE) : BLOCK_SIZE;
//         fwrite(contenido + i * BLOCK_SIZE, 1, bytes_a_escribir, bloques_file);

//         log_info(LOGGER_FILESYSTEM, "## Bloque asignado: %d - Archivo: %s - Bloques Libres: %d", bloque_datos, bloques_path, obtener_bloques_libres());
//         log_info(LOGGER_FILESYSTEM, "## Acceso Bloque - Archivo: %s - Tipo Bloque: DATOS - Bloque File System: %d", bloques_path, bloque_datos);
//     }

//     fclose(bloques_file);
// }

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
            log_info(LOGGER_FILESYSTEM, "Bloque %d asignado.", i);
            return i;
        }
    }

    log_error(LOGGER_FILESYSTEM, "No hay bloques libres disponibles.");
    return -1; // NO SE ENCONTRO NINGUN BLOQUE LIBRE
}

int hay_espacion_suficiente(int tamanio_archivo) {
    int bloques_necesarios = (tamanio_archivo + BLOCK_SIZE - 1) / BLOCK_SIZE;
    int bloques_libres = obtener_bloques_libres();

    if (bloques_libres >= bloques_necesarios) {
        return 1;
    }

    log_warning(LOGGER_FILESYSTEM, "No hay suficiente espacio. Bloques necesarios: %d, Bloques libres: %d", bloques_necesarios, bloques_libres);
    return 0;
}


int obtener_bloques_libres() {
    if (!bitmap_memoria) {
        log_error(LOGGER_FILESYSTEM, "Bitmap no está cargado en memoria.");
        return -1;
    }

    int bloques_libres = 0;
    for (int i = 0; i < BLOCK_COUNT; i++) {
        if (!(bitmap_memoria[i / 8] & (1 << (i % 8)))) {
            bloques_libres++;
        }
    }

    log_info(LOGGER_FILESYSTEM, "Bloques libres: %d", bloques_libres);
    return bloques_libres;
}

// LIBERAR BLOQUES (OPCIONAL)
void liberar_bloque(int numero_bloque) {
    if (!bitmap_memoria || numero_bloque < 0 || numero_bloque >= BLOCK_COUNT) {
        log_error(LOGGER_FILESYSTEM, "Bloque inválido o bitmap no cargado en memoria. Bloque: %d", numero_bloque);
        return;
    }

    unsigned char byte = bitmap_memoria[numero_bloque / 8];
    if (!(byte & (1 << (numero_bloque % 8)))) {
        log_warning(LOGGER_FILESYSTEM, "El bloque %d ya está libre.", numero_bloque);
        return;
    }

    bitmap_memoria[numero_bloque / 8] &= ~(1 << (numero_bloque % 8));
    log_info(LOGGER_FILESYSTEM, "Bloque %d liberado.", numero_bloque);
}

int crear_archivo_dump(char* nombre_archivo, char* contenido, int tamanio) {
    if (!hay_espacion_suficiente(tamanio)) {
        log_error(LOGGER_FILESYSTEM, "No hay espacio suficiente para el archivo: %s", nombre_archivo);
        return -1;
    }

    int bloque_indice = asignar_bloque();
    if (bloque_indice == -1) {
        log_error(LOGGER_FILESYSTEM, "Error al asignar bloque de índice");
        return -1;
    }

    char files_path[256];
    sprintf(files_path, "%s/files", MOUNT_DIR);
    if (access(files_path, F_OK) != 0 && mkdir(files_path, 0755) == -1) {
        log_error(LOGGER_FILESYSTEM, "Error al crear el directorio files");
        liberar_bloque(bloque_indice);
        return -1;
    }

    char metadata_path[256];
    //sprintf(metadata_path, "%s/%s.dmp", files_path, nombre_archivo);
    int ret = snprintf(metadata_path, sizeof(metadata_path), "%s/%s.dmp", files_path, nombre_archivo);
    if (ret < 0 || ret >= sizeof(metadata_path)) {
        log_error(LOGGER_FILESYSTEM, "Error: La ruta generada es demasiado larga.");
        return -1;
    }

    FILE* metadata = fopen(metadata_path, "w+");
    if (!metadata) {
        log_error(LOGGER_FILESYSTEM, "Error al crear archivo de metadata: %s", metadata_path);
        liberar_bloque(bloque_indice);
        return -1;
    }

    fprintf(metadata, "SIZE=%d\nINDEX_BLOCK=%d\n", tamanio, bloque_indice);
    fclose(metadata);

    escribir_en_bloques(contenido, tamanio, bloque_indice);
    log_info(LOGGER_FILESYSTEM, "Archivo creado: %s - Tamaño: %d", nombre_archivo, tamanio);
    return 1;
}

void escribir_en_bloques(char* contenido, int tamanio, int bloque_indice) {
    char bloques_path[256];
    sprintf(bloques_path, "%s/bloques.dat", MOUNT_DIR);
    FILE* bloques_file = fopen(bloques_path, "r+");
    if (!bloques_file) {
        log_error(LOGGER_FILESYSTEM, "Error al abrir bloques.dat");
        liberar_bloque(bloque_indice);
        return;
    }

    int cantidad_bloques = (tamanio + BLOCK_SIZE - 1) / BLOCK_SIZE;

    for (int i = 0; i < cantidad_bloques; i++) {
        int bloque_datos = asignar_bloque();
        if (bloque_datos == -1) {
            log_error(LOGGER_FILESYSTEM, "Error al asignar bloque de datos.");
            liberar_bloque(bloque_indice);
            fclose(bloques_file);
            return;
        }

        usleep(RETARDO_ACCESO_BLOQUE * 1000);

        fseek(bloques_file, bloque_datos * BLOCK_SIZE, SEEK_SET);
        size_t bytes_a_escribir = (i == cantidad_bloques - 1) ? (tamanio % BLOCK_SIZE) : BLOCK_SIZE;
        fwrite(contenido + i * BLOCK_SIZE, 1, bytes_a_escribir, bloques_file);

        log_info(LOGGER_FILESYSTEM, "Bloque %d asignado. Bytes escritos: %zu", bloque_datos, bytes_a_escribir);
    }

    fclose(bloques_file);
}