/*
 * osada_utils.c
 *
 *  Created on: 23/10/2016
 *      Author: Facundo Castellano
 */

#include "osada_utils.h"

#include <asm-generic/errno-base.h>
#include <bits/mman-linux.h>
#include <commons/bitarray.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <commons/string.h>
#include <pthread.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>

t_info_file *getOsadaFile(char *path, int posicionPadre);
t_info_file *getInfoFile(char *path);
char *getNombreFile(char *subpath);
char *getPathPadre(char *subpath);
int delete(char *path, osada_file_state tipo);
bool hayDisponiblesBitmap(int cantidadBloques);
int getUltPosBlock(int primerBloque);
int countBlocks(int primerBloque);

void mapearDisco(int fd){

	struct stat sbuf;
	if(fstat(fd, &sbuf) == -1){
		log_error(log_serv, "El disco no se pudo abrir");
		exit(1);
	}

	disco = mmap(NULL, sbuf.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

	int offset = 0;

	//HEADER
	osadaDisco.header = disco;
	offset += OSADA_BLOCK_SIZE;

	//BITMAP
	char *bitarray = disco + offset;
	osadaDisco.bitmap = bitarray_create(bitarray, OSADA_BLOCK_SIZE * osadaDisco.header->bitmap_blocks);
	offset += OSADA_BLOCK_SIZE * osadaDisco.header->bitmap_blocks;

	//TABLA DE ARCHIVOS
	int tablaArchivosSize = BLOCKS_SIZE * OSADA_BLOCK_SIZE;
	osadaDisco.tablaArchivos = disco + offset;

	offset += tablaArchivosSize;
	//R/W
	int i = 0;
	for (i=0; i < TABLE_FILE_SIZE; i++) {
		pthread_rwlock_t lock;
		pthread_rwlock_init(&lock, NULL);
		locks[i] = lock;
	}

	//TABLA DE ASIGNACION
	int tablaAsignacionSize = (osadaDisco.header->fs_blocks - 1 - osadaDisco.header->bitmap_blocks - 1024) * 4;
	osadaDisco.tablaAsignacion = disco + offset;
	offset += tablaAsignacionSize + 4;

	//BLOQUES DE DATOS
	osadaDisco.dataBlock = disco + offset;

}

t_info_file *getOsadaFile(char *path, int posicionPadre){

	t_info_file *infoFile = NULL;
	unsigned char fneed[OSADA_FILENAME_LENGTH];
	memset(fneed, '\0', OSADA_FILENAME_LENGTH);
	memcpy(fneed, path, string_length(path));
	char *fname = malloc(sizeof(unsigned char) * OSADA_FILENAME_LENGTH);

	pthread_mutex_lock (&mutexTabla);
	//Recorro la tabla de archivos buscando el archivo que tenga ese nombre
	int i = 0;
	for(i=0; i<TABLE_FILE_SIZE;i++){

		osada_file *file = &osadaDisco.tablaArchivos[i];

		if(file != NULL){
			memcpy(fname, file->fname, OSADA_FILENAME_LENGTH);

			//Si el archivo tiene el mismo nombre y no esta borrado, lo asigno.
			if((string_equals_ignore_case(fname, path) || memcmp(file->fname, fneed, OSADA_FILENAME_LENGTH) == 0) && file->state != DELETED
					&& file->parent_directory == posicionPadre){
				free(fname);
				infoFile = malloc(sizeof(t_info_file));
				infoFile->osadaFile = file;
				infoFile->posicion = i;
				break;
			}
		}
	}
	pthread_mutex_unlock (&mutexTabla);

	return infoFile;
}

t_info_file *getInfoFile(char *path){

	t_info_file *infoFile = NULL;
	t_queue *colaArbol = queue_create();
	char **subpaths;

	//Si el path es /, estoy en el directorio raiz
	if(strcmp(path, "/") != 0){
		//Descompongo el path en distintos directorios.
		subpaths = string_split(path, "/");

		void cargarArbol(char *value){
			queue_push(colaArbol, value);
		}

		string_iterate_lines(subpaths, cargarArbol);
	}

	//El path tendria que empezar por el directorio raiz
	int posicionPadre = 0xFFFF;

	//Si el arbol esta vacio, es porque es el directorio raiz.
	if(queue_is_empty(colaArbol)){

		infoFile = malloc(sizeof(t_info_file));
		infoFile->posicion = posicionPadre;

	}else {

		//Voy buscando el archivo a traves de las ramas
		char *subpath;
		while(!queue_is_empty(colaArbol)){

			subpath = queue_pop(colaArbol);
			//El infoFile contendra el osada_file y la posicion en donde se encuentra.
			//En caso de que el archivo sea un directorio, su posicion sera la posicionPadre
			//del proximo subpath
			infoFile = getOsadaFile(subpath, posicionPadre);
			if(infoFile != NULL)
				posicionPadre = infoFile->posicion;
			else{
				free(subpath);
				break;
			}
			//Si no es el ultimo subpath, no necesito su infoFile. Lo libero
			if(!queue_is_empty(colaArbol))
				free(infoFile);

			free(subpath);
		}
		free(subpaths);
	}

	queue_destroy_and_destroy_elements(colaArbol, free);

	return infoFile;
}

/*
 * Retorna una lista de osada_files a partir de un path de directorio
 */
t_list *getListFiles(char *path){

	t_list *listaFiles = NULL;

	//Obtengo la info del file del directorio
	t_info_file *fileDirectory = getInfoFile(path);

	if(fileDirectory != NULL){

		listaFiles = list_create();
		//A partir de la posicion del directorio, traigo todos los files que se encuentra ahi
		int posicionDirectory = fileDirectory->posicion;

		pthread_mutex_lock (&mutexTabla);
		int i = 0;
		for(i=0; i<TABLE_FILE_SIZE;i++){

			osada_file *file = &osadaDisco.tablaArchivos[i];
			//Agrego todos los archivos que no esten borrados y tengan como padre al directorio
			if(file != NULL && file->parent_directory == posicionDirectory && file->state != 0){
				list_add(listaFiles, file);
			}
		}
		pthread_mutex_unlock (&mutexTabla);

		free(fileDirectory);
	}
	return listaFiles;
}
/*
 * Se crea un osada_file REGULAR y se le asigna una posicion en la tabla de archivos
 */
int newOsadaFile(char *path, osada_file_state tipo){

	osada_file *osadaFile = NULL;

	//Obtengo el nombre del path
	char *fname = getNombreFile(path);

	//Obtengo el path padre
	char *dname = getPathPadre(path);

	//Busco el info del directorio padre
	t_info_file *fileDirectory = getInfoFile(dname);

	free(dname);

	//Si existe el directorio padre, sigo
	if(fileDirectory != NULL){

		//Asigno una posicion libre en la tabla de archivos. Si no hay ninguna, osadaFile sera null
		osadaFile = asignarEnTablaDeArchivos(tipo, fname, fileDirectory->posicion);
		free(fileDirectory);
		free(fname);
		if(osadaFile != NULL)
			return OK;
		else
			return EDQUOT;

	}else{
		free(fname);
		return ENOENT;
	}

}

osada_file *asignarEnTablaDeArchivos(osada_file_state tipo, char *fname, int parentDirectory){

	osada_file *osadaFile = NULL;

	pthread_mutex_lock (&mutexTabla);
	int i = 0;
	for(i=0; i<TABLE_FILE_SIZE;i++){

		osada_file *file = &osadaDisco.tablaArchivos[i];
		//Creo un nuevo archivo si hay lugar (si hay alguno borrado)
		if(file == NULL || file->state == DELETED){
			if(file == NULL)
				file = malloc(sizeof(osada_file));

			memset(file->fname, '\0', OSADA_FILENAME_LENGTH);
			memcpy(file->fname, fname, string_length(fname));
			file->parent_directory = (uint16_t) parentDirectory;
			file->file_size = (uint32_t) 0;
			//Le cambio su fecha de modificacion
			file->lastmod = time(NULL);
			file->first_block = (osada_block_pointer) 0xFFFFFFFF;
			file->state = tipo;
			osadaFile = file;
			osadaDisco.tablaArchivos[i] = *file;
			break;
		}
	}
	pthread_mutex_unlock (&mutexTabla);

	return osadaFile;
}

int readFile(char *path, char *buf, int size, int offset){

	//Buscar el archivo
	osada_file *file = getFile(path);
	if(file == NULL){
		return -ENOENT;
	}else {
		//Lockeo el mutex read
		int posFile = getPosOsadaFile(file);
		pthread_rwlock_rdlock(&locks[posFile]);

		int bloqueInicial = offset / OSADA_BLOCK_SIZE;
		//Calculo la cantidad de bloques que necesitaria para usar el size
		int cantBloques = size/OSADA_BLOCK_SIZE;
		int bloqueFinal = bloqueInicial + cantBloques;
		//Saco el resto del bloque final. Si es mayor a 0, entonces sumo un bloque mas
		if((size % OSADA_BLOCK_SIZE) > 0)
			bloqueFinal++;
		//Calculo el size del block
		int sizeBlock = bloqueFinal - bloqueInicial > 1? bloqueFinal - bloqueInicial:1;
		//Busco la posicion del bloque desde donde tengo que empezar a leer
		int initPos = getPosBlock(file, bloqueInicial);
		int sizeRead = 0;
		//Capaz el archivo no tiene ningun bloque asignado
		if(initPos != -1)
			sizeRead= getBlocks(initPos, buf, sizeBlock);

		pthread_rwlock_unlock(&locks[posFile]);
		return sizeRead;
	}
}

int writeFile(char *path, char *buf, size_t size, off_t offset){

	//Busco el file
	osada_file *file = getFile(path);

	//Me fijo si tiene asignado algun bloque padre. Si no es asi, reservo uno y se lo asigno.
	if(file->first_block == 0xFFFFFFFF){

		file->first_block = (osada_block_pointer) reservarPosicionBitmap();
		if(file->first_block == -1){
			//Ya no hay posiciones disponibles en el bitmap
			return ENOSPC;

		}else
			osadaDisco.tablaAsignacion[file->first_block] =  0xFFFFFFFF;
	}
	int nroBloque = offset / OSADA_BLOCK_SIZE;
	//Obtengo la posicion del bloque donde tendria que empezar
	int posBlockPadre = getPosBlock(file, nroBloque);

	//Si no tiene un bloque asignado, reservo uno
	if(posBlockPadre == -1){
		posBlockPadre = reservarPosicionBitmap();
		//Lo enlazo
		int ultimoBloque = getUltPosBlock(file->first_block);
		osadaDisco.tablaAsignacion[ultimoBloque] = posBlockPadre;
		osadaDisco.tablaAsignacion[posBlockPadre] =  0xFFFFFFFF;
	}

	int offsetRelativo = offset % OSADA_BLOCK_SIZE;
	int sizeBlockDisponible = OSADA_BLOCK_SIZE - offsetRelativo;
	int sizeNecesitado = size > sizeBlockDisponible? sizeBlockDisponible:size;

	//Lockeo el mutex write
	int posFile = getPosOsadaFile(file);
	pthread_rwlock_wrlock(&locks[posFile]);

	//Cargo la data que entra en el primer bloque
	memcpy(&osadaDisco.dataBlock[posBlockPadre * OSADA_BLOCK_SIZE] + offsetRelativo, buf, sizeNecesitado);

	buf += sizeNecesitado;

	int sizeRelativo = size - sizeNecesitado;

	//Si no entra en el block que me trae, entonces tendria que reservar los bits, agregarlo a la tabla de asignacion y copiar la data
	if(sizeRelativo > 0){

		int i;
		int posBlockHijo;
		//Calculo la cantidad de blocks a partir del size necesitado.
		int cantidadBloques = sizeRelativo / OSADA_BLOCK_SIZE;
		if((sizeRelativo % OSADA_BLOCK_SIZE) > 0)
			cantidadBloques++;
		int sizeDatos;

		//Verifico si hay disponibles la cantidad de bloques que necesita
		if(!hayDisponiblesBitmap(cantidadBloques)){
			pthread_rwlock_unlock(&locks[posFile]);
			return ENOSPC;
		}

		for(i=0; i<cantidadBloques; i++){

			posBlockHijo = reservarPosicionBitmap();
			if(posBlockHijo != -1){
				//Calculo la cantidad de datos a copiar
				if(sizeRelativo > OSADA_BLOCK_SIZE){
					sizeDatos = OSADA_BLOCK_SIZE;
					sizeRelativo -= OSADA_BLOCK_SIZE;
				}else{
					sizeDatos = sizeRelativo;
				}
				//Hay que enlazar en la tabla de asignaciones.
				osadaDisco.tablaAsignacion[posBlockPadre] = posBlockHijo;
				memcpy(&osadaDisco.dataBlock[posBlockHijo * OSADA_BLOCK_SIZE], buf, sizeDatos);
				buf += sizeDatos;
				//Hago el memcpy  de lo que entre en ese block
				posBlockPadre = posBlockHijo;
			}
		}
		//Le pongo el fin de bloque en la tabla de asignaciones
		osadaDisco.tablaAsignacion[posBlockPadre] = (osada_block_pointer) 0xFFFFFFFF;
	}
	//Le cambio el size al archivo
	file->file_size = (uint32_t) size + offset;
	//Le cambio la fecha de modificacion
	file->lastmod = time(NULL);

	pthread_rwlock_unlock(&locks[posFile]);

	return OK;

}

void liberarBloques(int cantBloques, osada_file* file) {

	int posicionesBloques[cantBloques];

	getPosBlocks(posicionesBloques, file->first_block, cantBloques);

	pthread_mutex_lock (&mutexBitmap);
	//Busco en el bitmap las posiciones y los libero
	int i;
	for (i = 0; i < cantBloques; i++) {
		liberarBitmap(posicionesBloques[i]);
	}
	pthread_mutex_unlock (&mutexBitmap);
}
int delete(char *path, osada_file_state tipo){

	if(tipo == DIRECTORY)
		return deleteDirectory(path);
	else
		return deleteFile(path);

}
int deleteFile(char *path){

	//Busco el archivo
	osada_file *file = getFile(path);

	if(file == NULL){
		return ENOENT;
	}

	//A partir del primer bloque y la cantidad todal, obtengo un array de posiciones de cada bloque usado por el archivo.
	int cantBloques = file->file_size / OSADA_BLOCK_SIZE;
	//Saco el resto de la cantidad de bloques total. Si es mayor a 0, entonces sumo un bloque mas
	if((file->file_size % OSADA_BLOCK_SIZE) > 0)
		cantBloques++;

	liberarBloques(cantBloques, file);
	//Lo paso a estado DELETED
	file->state = DELETED;

	return OK;
}

int deleteDirectory(char *path){

	osada_file *file = getFile(path);

	if(file == NULL || file->state != DIRECTORY){
		return ENOENT;
	}

	t_list *listaFiles = getListFiles(path);
	if(!list_is_empty(listaFiles)){
		//No puede estar con archivos
		return EPERM;
	}else{
		file->state = DELETED;
		//setOsadaFile(path, file);
		return OK;
	}

}

int renameFile(char *path, char *newPath){

	if(string_equals_ignore_case(path, newPath))
		return EPERM;

	osada_file *file = getFile(path);

	if(file != NULL){

		//Obtengo el path padre del new path
		char *newPathPadre = getPathPadre(newPath);

		//Busco el info del directorio padre
		t_info_file *fileDirectory = getInfoFile(newPathPadre);

		if(fileDirectory == NULL){
			return ENOENT;
		}
		//Busco el archivo del nuevo path
		osada_file *newFile = getFile(newPath);
		if(newFile != NULL){
			//Lo borro porque lo tengo que reemplazar
			delete(newPath, newFile->state);
		}

		char *newName = getNombreFile(newPath);

		//Cambio el nombre y la posicion de su padre
		memcpy(file->fname, newName, OSADA_FILENAME_LENGTH);
		free(newName);
		file->parent_directory = (uint16_t) fileDirectory->posicion;
		//Le cambio su fecha de modificacion
		file->lastmod = time(NULL);
		//setOsadaFile(newPath, file);
		return OK;
	}else
		return ENOENT;

}

int truncateOsadaFile(char *path, int size){

	int posBlockPadre;
	int posBlockHijo;
	int i;

	//Busco el file
	osada_file *file = getFile(path);

	if(file == NULL)
		return ENOENT;

	//Calculo los bloques a partir del size
	int cantBloques = size/OSADA_BLOCK_SIZE;
	//Saco el resto de la cantidad de bloques. Si es mayor a 0, entonces sumo un bloque mas
	if((size % OSADA_BLOCK_SIZE) > 0)
		cantBloques++;

	//Si no tiene bloques asignados, le asigno la cantidad de bloques que pidio
	if(file->first_block == 0xFFFFFFFF){

		if(!hayDisponiblesBitmap(cantBloques))
			return EFBIG;

		//Reservo el primer bloque
		file->first_block = (osada_block_pointer) reservarPosicionBitmap();
		osadaDisco.tablaAsignacion[file->first_block] =  0xFFFFFFFF;
		memset(&osadaDisco.dataBlock[file->first_block * OSADA_BLOCK_SIZE], '\0', OSADA_BLOCK_SIZE);

		posBlockPadre = file->first_block;
		posBlockHijo = 0;
		i = 0;
		//Reservo las posiciones del bitmap
		for(i=0; i<cantBloques; i++){
			posBlockHijo = reservarPosicionBitmap();
			if(posBlockHijo != -1){

				//Hay que enlazar en la tabla de asignaciones.
				osadaDisco.tablaAsignacion[posBlockPadre] = posBlockHijo;
				//Limpio el buffer
				memset(&osadaDisco.dataBlock[posBlockHijo * OSADA_BLOCK_SIZE], '\0', OSADA_BLOCK_SIZE);

				posBlockPadre = posBlockHijo;
			}
		}

	}else{
		//El archivo ya tiene bloques asignados, tengo que buscarlos
		//Obtengo la cantidad de bloques asignados del archivo
		int cantidadAsignada = countBlocks(file->first_block);

		if(cantidadAsignada < size){

			//Tengo que asignarle los bloques que faltan
			int bloquesFaltantes = size - cantidadAsignada;
			//Busco la posicion del ultimo bloque
			int posUltimoBloque = getUltPosBlock(file->first_block);

			if(!hayDisponiblesBitmap(bloquesFaltantes))
				return EFBIG;
			//Asigno los bloques faltantes a partir del ultimo bloque
			posBlockPadre = posUltimoBloque;
			posBlockHijo = 0;
			i = 0;
			//Reservo las posiciones del bitmap
			for(i=0; i<bloquesFaltantes; i++){
				posBlockHijo = reservarPosicionBitmap();
				if(posBlockHijo != -1){

					//Hay que enlazar en la tabla de asignaciones.
					osadaDisco.tablaAsignacion[posBlockPadre] = posBlockHijo;
					//Limpio el buffer
					memset(&osadaDisco.dataBlock[posBlockHijo * OSADA_BLOCK_SIZE], '\0', OSADA_BLOCK_SIZE);

					posBlockPadre = posBlockHijo;
				}
			}
		}else{

			//Tengo que liberar los bloques que sobran
			//8 asignados, solo necesito 5 = 3 que sobran
			int bloquesSobrantes = cantidadAsignada - size;
			int posDesde = getPosBlock(file, size);
			int bloquesAsignados[bloquesSobrantes];
			getPosBlocks(bloquesAsignados, posDesde, bloquesSobrantes);//[0,5,9,10,11,12,13,14]

			if(posDesde == file->first_block)
				file->first_block = 0xFFFFFFFF;
			//Busco en el bitmap las posiciones y los libero
			i = 0;
			for (i = size; i < bloquesSobrantes; i++) {
				liberarBitmap(bloquesAsignados[i]);
				osadaDisco.tablaAsignacion[bloquesAsignados[i]] = 0xFFFFFFFF;
			}
		}
	}

	file->file_size = size;
	file->lastmod = time(NULL);

	return 0;

}

int utimensOsadaFile(char *path, long time){

	//Busco el file
	osada_file *file = getFile(path);

	if(file == NULL)
		return ENOENT;

	//Actualizo su ultima modificacion
	file->lastmod = (uint32_t) time;

	return OK;
}

int getBlocks(int initPos,char *blocks, int sizeBlock){

	int posicionBloque = initPos;
	int offset = 0;
	int sizeRead = 0;
	int i = 0;
	for(i=0;i<sizeBlock;i++){
		//Copio el bloque de esa posicion
		memcpy(blocks + offset, &osadaDisco.dataBlock[posicionBloque * OSADA_BLOCK_SIZE], OSADA_BLOCK_SIZE);
		sizeRead += OSADA_BLOCK_SIZE;
		posicionBloque = osadaDisco.tablaAsignacion[posicionBloque];
		//Si el file no tiene mas bloques asignados, dejo de leer
		if(posicionBloque == 0xFFFFFFFF)
			break;
		else
			offset += OSADA_BLOCK_SIZE;
	}

	return sizeRead;
}

int getPosBlock(osada_file *file, int cantidadBloques){

	int posicion = file->first_block;
	int i;

	if(posicion != -1){
		for(i=0;i<cantidadBloques;i++){
			posicion = osadaDisco.tablaAsignacion[posicion];
		}
	}

	return posicion;

}

void getPosBlocks(int *blocks,int primerBloque, int cantidadBlocks){

	int posicion = primerBloque;
	int i;

	for(i=0;i<cantidadBlocks;i++){
		blocks[i] = posicion;
		posicion = osadaDisco.tablaAsignacion[posicion];
	}

}

int getUltPosBlock(int primerBloque){

	int ultPosBlock, bloque = primerBloque;

	while(bloque != 0xFFFFFFFF){
		bloque = osadaDisco.tablaAsignacion[bloque];
		if(bloque != 0xFFFFFFFF)
			ultPosBlock = bloque;
	}

	return ultPosBlock;
}

int countBlocks(int primerBloque){

	int cantidadBloques = 0;
	int bloque = primerBloque;

	while(bloque != 0xFFFFFFFF){
		cantidadBloques++;
		bloque = osadaDisco.tablaAsignacion[bloque];
	}

	return cantidadBloques;
}

void liberarBitmap(int posicion){

	bitarray_clean_bit(osadaDisco.bitmap, posicion);
}

void ocuparBitmap(int posicion){
	bitarray_set_bit(osadaDisco.bitmap, posicion);
}

int getPosLibreBitmap(){

	int pos = -1;
	int i= 0;
	int sizeBitmap = osadaDisco.bitmap->size * 8;
	for(i=0;i<sizeBitmap;i++){
		if(!bitarray_test_bit(osadaDisco.bitmap, i)){
			pos = i;
			break;
		}
	}
	return pos;
}

int reservarPosicionBitmap(){

	pthread_mutex_lock (&mutexBitmap);
	int pos = getPosLibreBitmap();

	if(pos != -1)
		ocuparBitmap(pos);

	pthread_mutex_unlock (&mutexBitmap);

	return pos;

}

bool hayDisponiblesBitmap(int cantidadBloques){

	int cantidadLibre = 0;
	int i= 0;
	int sizeBitmap = osadaDisco.bitmap->size * 8;
	pthread_mutex_lock (&mutexBitmap);
	for(i=0;i<sizeBitmap;i++){

		if(cantidadLibre == cantidadBloques){
			pthread_mutex_unlock (&mutexBitmap);
			return true;
		}

		if(!bitarray_test_bit(osadaDisco.bitmap, i)){
			cantidadLibre++;
		}
	}

	pthread_mutex_unlock (&mutexBitmap);
	return false;
}

/*
 * FUNCIONES PRIVADAS
 */
osada_file *getFile(char *path){

	osada_file *osadaFile = NULL;
	t_info_file *infoFile = getInfoFile(path);

	if(infoFile != NULL){
		osadaFile = infoFile->osadaFile;
		free(infoFile);
	}

	return osadaFile;
}

bool isExist(char *path , int tipo){


	osada_file *osadaFile = getFile(path);

	return osadaFile != NULL && osadaFile->state == tipo;
}

char *getNombreFile(char *path){

	char *nombreFile = malloc(sizeof(char) * OSADA_FILENAME_LENGTH);
	char** subpath = string_split(path, "/");

	void closure(char *value){
		free(nombreFile);
		nombreFile = value;
	}

	string_iterate_lines(subpath, closure);

	free(subpath);

	if(string_ends_with(nombreFile, "/")){
		char *subNombreFile = string_substring_until(nombreFile, string_length(nombreFile) - 1);
		free(nombreFile);
		nombreFile = subNombreFile;
	}

	return nombreFile;
}

char *getPathPadre(char *path){

	char** subpath = string_split(path, "/");

	char *pathPadre = string_new();
	char *subpathPadre = string_new();
	t_queue *colaPathPadre = queue_create();

	void closure(char *value){
		queue_push(colaPathPadre, value);
	}
	string_iterate_lines(subpath, closure);

	while(queue_size(colaPathPadre)>1){
		char *value = queue_pop(colaPathPadre);
		string_append_with_format(&subpathPadre,"%s/", value);
		free(pathPadre);
		pathPadre = string_substring_until(subpathPadre, string_length(subpathPadre) - 1);
	}

	free(subpathPadre);
	queue_destroy_and_destroy_elements(colaPathPadre, free);

	free(subpath);

	return pathPadre;
}

int getPosOsadaFile(osada_file *unFile){

	pthread_mutex_lock (&mutexTabla);
	//Recorro la tabla de archivos buscando el archivo que tenga ese nombre
	int i = 0;
	for(i=0; i<TABLE_FILE_SIZE;i++){

		osada_file *file = &osadaDisco.tablaArchivos[i];

		if(file != NULL){

			if(unFile == file){
				pthread_mutex_unlock (&mutexTabla);
				return i;
			}
		}
	}
	pthread_mutex_unlock (&mutexTabla);
	return -1;
}

bool tieneBloquesAsignados(osada_file *file){

	return osadaDisco.tablaAsignacion[file->first_block] != -1;
}

/*int main(int argc, char **argv) {

	int asd = 5;
	char buffer[sizeof(int)];
	memcpy(buffer, &asd, sizeof(int));
}*/


