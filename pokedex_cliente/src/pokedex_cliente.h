/*
 * pokedex_cliente.h
 *
 *  Created on: 28/9/2016
 *      Author: utnso
 */

#ifndef POKEDEX_CLIENTE_H_
#define POKEDEX_CLIENTE_H_
#define _FILE_OFFSET_BITS  64
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stddef.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <err.h>
#include <time.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/time.h>
#include "commons/bitarray.h"
#include "commons/string.h"
#include "tipos.h"
#include "socketes_cliente.h"
#include "socketes_servidor.h"
#include "estructuras.h"
#include "osada.h"
#include <commons/log.h>
#include <commons/config.h>
#include <commons/collections/list.h>

t_log* pokedexLog;
t_socket_client* pokedexServer; //Socket Servidor

// PROTOTYPES

void obtenerContenidoDesdeHasta(const char path, off_t inicio, size_t fin);
void levantarArchivoMapa(t_msj_mapa* mapa);
//agregado PROTOYPES
osada_file *getOsadaFile(const char *path);
int crearOsadaFile(const char *path);
int crearOsadaDirectory(const char *path);
int eliminaOsadaFile(const char *path, int tipo);
t_list*  listaOsadaFile(const char *path);
int readOsadaFile(const char *path, char *buf, off_t inicio, size_t size);
int writeOsadaFile(const char *path, const char *buf, off_t offset, size_t size);
int renameOsadaFile(const char *path, const char *newPath);
int poke_unlink (const char *path);
int poke_rename (const char *path, const char *newPath);
int poke_truncate(const char *path, off_t size);
int truncateOsadaFile(const char *path, off_t size);
int poke_utimens(const char* path, const struct timespec ts[2]);
int utimensOsadaFile(const char* path, const struct timespec ts);

char *getNombreFile(char *path);
bool string_contains(char *path, char caracter);

#endif /* POKEDEX_CLIENTE_H_ */
