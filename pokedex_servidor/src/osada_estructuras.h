/*
 * osada_estructuras.h
 *
 *  Created on: 23/10/2016
 *      Author: Facundo Castellano
 */

#ifndef OSADA_ESTRUCTURAS_H_
#define OSADA_ESTRUCTURAS_H_

#include "osada.h"
#define OSADA_BLOCK_SIZE 64
#define OSADA_FILENAME_LENGTH 17
#define FILE_MAX_SIZE_BLOCK 67108864
#define TABLE_FILE_SIZE 2048
#define BLOCKS_SIZE 1024
#define OK			47

typedef struct {
	osada_file *osadaFile;
	int posicion;
}t_info_file;

typedef struct {
	char block[OSADA_BLOCK_SIZE];
	int proximaPos;
}t_info_block;

typedef struct {
	osada_header *header;
	t_bitarray *bitmap;
	osada_file *tablaArchivos;
	int *tablaAsignacion;
	char *dataBlock;
}t_osada_disco;

t_osada_disco osadaDisco;
void *disco;
int fd;

pthread_mutex_t mutexTabla;
pthread_mutex_t mutexBitmap;
t_log *log_serv;

pthread_rwlock_t locks[TABLE_FILE_SIZE];

#endif /* OSADA_ESTRUCTURAS_H_ */
