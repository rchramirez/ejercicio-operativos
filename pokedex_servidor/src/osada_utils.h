/*
 * osada_utils.h
 *
 *  Created on: 23/10/2016
 *      Author: Facundo Castellano
 */

#ifndef OSADA_UTILS_H_
#define OSADA_UTILS_H_

#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <commons/bitarray.h>
#include <commons/collections/queue.h>
#include <commons/string.h>
#include <commons/log.h>
#include "osada_estructuras.h"

void mapearDisco(int fd);

osada_file *getFile(char *path);
t_list *getListFiles(char *path);

int readFile(char *path, char *buf, int size, int offset);
int writeFile(char *path, char *buf, size_t size, off_t offset);

int renameFile(char *path, char *newPath);

osada_file *asignarEnTablaDeArchivos(osada_file_state tipo, char *fname, int parentDirectory);

int getBlocks(int initPos,char *blocks, int sizeBlock);
void getPosBlocks(int *blocks,int primerBloque, int blockSize);
void liberarBitmap(int posicion);

int asignarBlock(int posicionPadre);
int reservarPosicionBitmap();
int getPosBlock(osada_file *file, int cantidadBloques);
bool isExist(char *path , int tipo);

int newOsadaFile(char *path, osada_file_state tipo);
int deleteFile(char *path);
int deleteDirectory(char *path);
int truncateOsadaFile(char *path, int size);
int utimensOsadaFile(char *path, long time);

int getPosOsadaFile(osada_file *unFile);
bool tieneBloquesAsignados(osada_file *file);

#endif /* OSADA_UTILS_H_ */
