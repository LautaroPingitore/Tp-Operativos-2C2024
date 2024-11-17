#ifndef MANEJOARCHIVOS_H_
#define MANEJOARCHIVOS_H_

int crear_archivo_dump(char*, char*, int);
void escribir_en_bloques(char*, int, int);
int asignar_bloque();
int hay_espacion_suficiente(int);
int obtener_bloques_libres();
void liberar_bloque(int);

#endif /* MANEJOARCHIVOS_H_*/