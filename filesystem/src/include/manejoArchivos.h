#ifndef MANEJOARCHIVOS_H_
#define MANEJOARCHIVOS_H_

extern char* bitmap_memoria;
extern 

void inicializar_archivo(char*, size_t, char*);
void limpiar_carpeta_files(const char*);
void iniciar_archivos();
int crear_archivo_dump(char*, char*, int);
void escribir_en_bloques(char*, int, int);
int asignar_bloque();
int hay_espacion_suficiente(int);
int obtener_bloques_libres();
void liberar_bloque(int);
void cargar_bitmap();
void guardar_bitmap();
void actualizar_bitmap();
void actualizar_bloques_libres_cache();

#endif /* MANEJOARCHIVOS_H_*/