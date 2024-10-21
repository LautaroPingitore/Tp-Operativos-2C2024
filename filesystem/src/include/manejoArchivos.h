#ifndef MANEJOARCHIVOS_H_
#define MANEJOARCHIVOS_H_

#include <utils/hello.h>

int crear_archivo_dump(char*, char*, int);
void escribir_en_bloques(char*, int, int);
int asignar_bloque();
int hay_espacion_suficiente(int);
void liberar_bloque(int);

#endif /* #define MANEJOARCHIVOS_H_*/