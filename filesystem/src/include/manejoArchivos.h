#ifndef MANEJOARCHIVOS_H_
#define MANEJOARCHIVOS_H_

extern char* bitmap_memoria;

void inicializar_archivo(char*, size_t, char*);
void iniciar_archivos();
int crear_archivo_dump(char*, char*, int);
void escribir_en_bloques(char*, int, int);
int asignar_bloque();
int hay_espacion_suficiente(int);
int obtener_bloques_libres();
void liberar_bloque(int);
void cargar_bitmap();
void guardar_bitmap();

#endif /* MANEJOARCHIVOS_H_*/