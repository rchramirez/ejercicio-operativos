/*
 * socketes.c
 *
 *  Created on: 04/09/2016
 *      Author: Facundo Borda
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <commons/log.h>
#include <string.h>
#include <commons/string.h>
#include "socketes.h"


/**
 * @NAME: socketCreate
 * @DESC: Crea un socket y devuelve un struct con el estado de creacion y el descriptor correspondiente.
 *
 * CREAR UN SOCKET:
 *
 * AF_INET: SOCKET DE INTERNET IPV4
 * SOCK_STREAM: ORIENTADO A LA CONEXION (TCP)
 * 0: USAR PROTOCOLO POR DEFECTO PARA AF_INET-SOCK_STREAM: PROTOCOLO TCP/IPV4
 *
 */

t_socket* socketCreate() {

	t_socket* ptrNewSocket;
	Int32U socketDescriptor;

	if ((socketDescriptor = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		return NULL;
	}

	ptrNewSocket = (t_socket *) malloc(sizeof(t_socket));
	ptrNewSocket->descriptor = socketDescriptor;

	return ptrNewSocket;

}

void closeSocket(t_socket *cliente){
	close(cliente->descriptor);
}

t_socket* socketGetServerFromAddress(struct sockaddr_in socketAddress) {

	t_socket *ptrSocketServer = (t_socket *) malloc(sizeof(t_socket));

	ptrSocketServer->ptrAddress = (struct sockaddr_in *) malloc(
			sizeof(struct sockaddr_in));
	ptrSocketServer->ptrAddress->sin_family = socketAddress.sin_family;
	ptrSocketServer->ptrAddress->sin_addr.s_addr =
			socketAddress.sin_addr.s_addr;
	ptrSocketServer->ptrAddress->sin_port = socketAddress.sin_port;

	return ptrSocketServer;
}

void enviarMsjEntrenador(t_socket *descriptor, t_msj_entrenador *msjEntrenador){

	serializarEntrenador(descriptor, msjEntrenador);
}

t_msj_entrenador *recibirMsjEntrenador(t_socket *descriptor){

	t_msj_entrenador *msj = NULL;
	//Primero recibo el size
	int size = recibirSize(descriptor);
	//Creo un buffer a partir del size
	if(size == 0)
		return msj;
	else{
		char buffer[size];
		recibirBuffer(descriptor, buffer, size);
		msj = deserializarEntrenador(buffer);
	}
	return msj;
}

void enviarPokemon(t_socket *descriptor, t_info_pokemon *pokemon){

	serializarPokemon(descriptor,pokemon);
}

t_info_pokemon *recibirPokemon(t_socket *descriptor){

	t_info_pokemon *msj = NULL;
	//Primero recibo el size
	int size = recibirSize(descriptor);
	//Creo un buffer a partir del size
	if(size == 0)
		return msj;
	else{
		char buffer[size];
		recibirBuffer(descriptor, buffer, size);
		msj = deserializarPokemon(buffer);
	}
	return msj;
}

void serializarEntrenador(t_socket *descriptor, t_msj_entrenador *msjEntrenador){

	int lengthNombre = string_length(msjEntrenador->nombre);

	int sizeBuffer = sizeof(t_msj_entrenador) + (sizeof(char) * lengthNombre + 1) + sizeof(int);
	char buffer[sizeBuffer + sizeof(int)];
	int size = 0;

	size = serializarIntSize(buffer, &sizeBuffer, size);
	size = serializarCharSize(buffer, &(msjEntrenador->simbolo), size);
	size = serializarIntSize(buffer, &(msjEntrenador->vidas), size);
	size = serializarIntSize(buffer, &(msjEntrenador->reintentos), size);
	size = serializarIntSize(buffer, &(msjEntrenador->accion), size);
	size = serializarIntSize(buffer, &(msjEntrenador->posicionX), size);
	size = serializarIntSize(buffer, &(msjEntrenador->posicionY), size);
	size = serializarIntSize(buffer, &(msjEntrenador->movimiento), size);
	size = serializarCharSize(buffer, &(msjEntrenador->pokemon), size);
	size = serializarIntSize(buffer, &(lengthNombre), size);

	memcpy(buffer + size, msjEntrenador->nombre, lengthNombre + 1);

	enviarBuffer(descriptor, buffer, sizeBuffer + sizeof(int));

}

t_msj_entrenador *deserializarEntrenador(char *buffer){

	t_msj_entrenador *msjEntrenador = malloc(sizeof(t_msj_entrenador));

	int size = 0;
	size = deserializarCharSize(buffer, &(msjEntrenador->simbolo), size);
	size = deserializarIntSize(buffer, &(msjEntrenador->vidas), size);
	size = deserializarIntSize(buffer, &(msjEntrenador->reintentos), size);
	size = deserializarIntSize(buffer, &(msjEntrenador->accion), size);
	size = deserializarIntSize(buffer, &(msjEntrenador->posicionX), size);
	size = deserializarIntSize(buffer, &(msjEntrenador->posicionY), size);
	size = deserializarIntSize(buffer, &(msjEntrenador->movimiento), size);
	size = deserializarCharSize(buffer, &(msjEntrenador->pokemon), size);

	int lengthNombre = 0;
	size = deserializarIntSize(buffer, &lengthNombre, size);

	msjEntrenador->nombre = malloc(sizeof(char) * lengthNombre + 1);
	memcpy(msjEntrenador->nombre, buffer + size, lengthNombre + 1);

	return msjEntrenador;
}

void serializarPokemon(t_socket *descriptor, t_info_pokemon *pokemon){

	int lengthNombreArchivo = string_length(pokemon->nombreArchivo);
	int lengthNombrePokemon = string_length(pokemon->nombrePokemon);

	int sizeBuffer = sizeof(t_info_pokemon) + (sizeof(char) * lengthNombreArchivo + 1)
			+ (sizeof(char) * lengthNombrePokemon + 1) + (sizeof(int) * 2);

	char buffer[sizeBuffer + sizeof(int)];
	int size = 0;

	size = serializarIntSize(buffer, &sizeBuffer, size);
	size = serializarIntSize(buffer, &(pokemon->nivel), size);

	size = serializarIntSize(buffer, &(lengthNombreArchivo), size);
	memcpy(buffer + size, pokemon->nombreArchivo, lengthNombreArchivo + 1);
	size += lengthNombreArchivo + 1;

	size = serializarIntSize(buffer, &(lengthNombrePokemon), size);
	memcpy(buffer + size, pokemon->nombrePokemon, lengthNombrePokemon + 1);

	enviarBuffer(descriptor, buffer, sizeBuffer + sizeof(int));
}

t_info_pokemon *deserializarPokemon(char *buffer){

	t_info_pokemon *pokemon = malloc(sizeof(t_info_pokemon));
	int size = 0;
	size = deserializarIntSize(buffer, &(pokemon->nivel), size);

	int lengthNombreArchivo = 0;
	size = deserializarIntSize(buffer, &lengthNombreArchivo, size);
	pokemon->nombreArchivo = malloc(sizeof(char) * lengthNombreArchivo + 1);
	memcpy(pokemon->nombreArchivo, buffer + size, lengthNombreArchivo + 1);
	size += lengthNombreArchivo + 1;

	int lengthNombrePokemon = 0;
	size = deserializarIntSize(buffer, &lengthNombrePokemon, size);
	pokemon->nombrePokemon = malloc(sizeof(char) * lengthNombrePokemon + 1);
	memcpy(pokemon->nombrePokemon, buffer + size, lengthNombrePokemon + 1);

	return pokemon;
}

void enviarMsjPokedex(t_socket *descriptor, t_msj_pokedex *mensaje){
	serializarMensajePokedex(descriptor,mensaje);
}

t_msj_pokedex *recibirMsjPokedex(t_socket *descriptor){

	t_msj_pokedex *msj = NULL;

	//Primero recibo el size
	int size = recibirSize(descriptor);
	//Creo un buffer a partir del size
	if(size == 0)
		return msj;
	else{
		char buffer[size];
		recibirBuffer(descriptor, buffer, size);
		msj = deserializarMensajePokedex(buffer);
	}
	return msj;

}

void enviarMsjReaddir(t_socket *descriptor, t_list *files, int res){

	int cantidadFiles = list_size(files);
	//Calculo el size
	int sizeBuf = (cantidadFiles * SIZE_OF_INFO_FILE) + (sizeof(int) * 2);
	char buffer[sizeBuf + sizeof(int)];
	int sizeAux = 0;
	//Le paso el size del buffer
	sizeAux = serializarIntSize(buffer, &sizeBuf, sizeAux);
	//Le paso el resultado
	sizeAux = serializarIntSize(buffer, &res, sizeAux);
	//Le paso la cantidad de files
	sizeAux = serializarIntSize(buffer, &cantidadFiles, sizeAux);
	serializarListaOsada(buffer, files, sizeAux);
	enviarBuffer(descriptor, buffer, sizeBuf + sizeof(int));
}

t_msj_readdir *recibirMsjReaddir(t_socket *descriptor){

	t_msj_readdir *msj = NULL;
	//Primero recibo el size
	int size = recibirSize(descriptor);
	//Creo un buffer a partir del size
	if(size == 0)
		return msj;
	else{
		char buffer[size];
		recibirBuffer(descriptor, buffer, size);
		int sizeAux = 0;
		msj = malloc(sizeof(t_msj_readdir));
		sizeAux = deserializarIntSize(buffer, &(msj->res), sizeAux);
		int cantidadFiles = 0;
		sizeAux = deserializarIntSize(buffer, &cantidadFiles, sizeAux);
		msj->list = deserializarListaOsada(buffer, cantidadFiles, sizeAux);
	}
	return msj;
}

void enviarMsjGetattr(t_socket *descriptor, osada_file *file, int res){

	//Calculo el size

	int sizeBuf = 0;
	if(res == OK)
		sizeBuf = SIZE_OF_INFO_FILE + sizeof(int);
	else
		sizeBuf = sizeof(int);

	char buffer[sizeBuf + sizeof(int)];

	int sizeAux = 0;

	//Le paso el size del buffer
	sizeAux = serializarIntSize(buffer, &sizeBuf, sizeAux);
	int elsize = 0;
	deserializarIntSize(buffer, &elsize, 0);
	//Le paso el resultado
	sizeAux = serializarIntSize(buffer, &res, sizeAux);
	if(res == OK){

		//Le paso el file
		serializarOsadaSize(buffer, file, sizeAux);
	}

	enviarBuffer(descriptor, buffer, sizeBuf + sizeof(int));
}

t_msj_getattr *recibirMsjGetattr(t_socket *descriptor){

	t_msj_getattr *msj = NULL;
	int size = recibirSize(descriptor);

	char buffer[size];
	recibirBuffer(descriptor, buffer, size);
	int sizeAux = 0;
	msj = malloc(sizeof(t_msj_readdir));
	sizeAux = deserializarIntSize(buffer, &(msj->res), sizeAux);
	if(msj->res == OK){

		t_socket_buffer *bufOsada = malloc(sizeof(t_socket_buffer));
		bufOsada->size = sizeAux;
		memcpy(bufOsada->data, buffer, size);

		msj->file = deserializarOsada(bufOsada);
		free(bufOsada);

	}else{
		msj->file = NULL;
	}

	return msj;
}

void enviarMsjRead(t_socket *descriptor, char *buf, int res, int size){

	//Calculo el size
	int sizeBuf;

	if(res == OK)
		sizeBuf = size + sizeof(int);
	else
		sizeBuf = sizeof(int);

	char buffer[sizeBuf + sizeof(int)];
	int sizeAux = 0;
	//Le paso el size del buffer
	sizeAux = serializarIntSize(buffer, &sizeBuf, sizeAux);
	//Le paso el resultado
	sizeAux = serializarIntSize(buffer, &res, sizeAux);

	if(res == OK)
		memcpy(buffer + sizeAux, buf, size);

	enviarBuffer(descriptor, buffer, sizeBuf + sizeof(int));
}

t_msj_read *recibirMsjRead(t_socket *descriptor, char *buf){

	t_msj_read *msj = malloc(sizeof(t_msj_read));

	int sizeBuff = recibirSize(descriptor);

	char buffer[sizeBuff];
	recibirBuffer(descriptor, buffer, sizeBuff);

	int sizeAux = 0;
	sizeAux = deserializarIntSize(buffer, &(msj->res), sizeAux);

	if(msj->res == OK){
		int sizeRead = sizeBuff - sizeAux;
		memcpy(buf, buffer + sizeAux, sizeRead);
		msj->sizeRead = sizeRead;
	}

	return msj;
}

/**
 * @NAME: socketDestroy
 * @DESC: Realiza el cierre de un determinado descriptor.
 * Retorna TRUE en caso de exito, FALSE caso contrario
 * @PARAMS:
 *		ptrSocket	: El descriptor a cerrar
 */
Boolean socketDestroy(t_socket *ptrSocket) {

	if (close(ptrSocket->descriptor) == 0) {
		return TRUE;
	}
	return FALSE;
}

int serializarCharSize(char *buffer, char* dato, int size){
	memcpy(buffer + size, dato, sizeof(char));
	return size + sizeof(char);
}
int deserializarCharSize(char *buffer, char* dato, int size){
	memcpy(dato, buffer + size, sizeof(char));
	return size + sizeof(char);
}

void serializarInt(t_socket_buffer *buffer, int* dato){
	int size = buffer->size;
	memcpy(buffer->data + size, dato, sizeof(int));
	buffer->size = buffer->size + sizeof(int);
}
void deserializarInt(t_socket_buffer *buffer, int* dato){
	int size = buffer->size;
	memcpy(dato, buffer->data + size, sizeof(int));
	buffer->size = buffer->size + sizeof(int);

}

int serializarIntSize(char *buffer, int* dato, int size){
	memcpy(buffer + size, dato, sizeof(int));
	return size + sizeof(int);
}

int deserializarIntSize(char *buffer, int *dato, int size){
	memcpy(dato, buffer + size, sizeof(int));
	return size + sizeof(int);
}

void serializarMensajePokedex(t_socket *descriptor,t_msj_pokedex *mensaje){

	int lengthPath = string_length(mensaje->path);
	int lengthNuevoNombre = string_length(mensaje->nuevoNombre);

	int sizeBuffer = sizeof(t_msj_pokedex) + (sizeof(char) * lengthPath + 1) + sizeof(int);

	if(lengthNuevoNombre > 0)
		sizeBuffer += (sizeof(char) * lengthNuevoNombre + 1) + sizeof(int);

	char buffer[sizeBuffer + sizeof(int)];
	int size = 0;

	size = serializarIntSize(buffer, &sizeBuffer, size);
	size = serializarIntSize(buffer, &(mensaje->accion), size);

	size = serializarIntSize(buffer, &(lengthPath), size);
	memcpy(buffer + size, mensaje->path, lengthPath + 1);
	size += lengthPath + 1;

	size = serializarIntSize(buffer, &(mensaje->off), size);
	size = serializarIntSize(buffer, &(mensaje->size), size);
	size = serializarLongSize(buffer, &(mensaje->time), size);

	size = serializarIntSize(buffer, &(lengthNuevoNombre), size);
	if(lengthNuevoNombre > 0)
		memcpy(buffer + size, mensaje->nuevoNombre, lengthNuevoNombre + 1);

	enviarBuffer(descriptor, buffer, sizeBuffer + sizeof(int));
}

t_msj_pokedex *deserializarMensajePokedex(char *buffer){

	t_msj_pokedex *mensaje = malloc(sizeof(t_msj_pokedex));

	int size = 0;
	size = deserializarIntSize(buffer, &(mensaje->accion), size);

	int lenghtPath = 0;
	size = deserializarIntSize(buffer, &lenghtPath, size);
	mensaje->path = malloc(sizeof(char) * lenghtPath + 1);
	memcpy(mensaje->path, buffer + size, lenghtPath + 1);
	size += lenghtPath + 1;

	size = deserializarIntSize(buffer, &(mensaje->off), size);
	size = deserializarIntSize(buffer, &(mensaje->size), size);
	size = deserializarLongSize(buffer, &(mensaje->time), size);

	int lenghtNuevoNombre = 0;
	size = deserializarIntSize(buffer, &lenghtNuevoNombre, size);
	if(lenghtNuevoNombre > 0){
		mensaje->nuevoNombre = malloc(sizeof(char) * lenghtNuevoNombre + 1);
		memcpy(mensaje->nuevoNombre, buffer + size, lenghtNuevoNombre + 1);
		size += lenghtNuevoNombre + 1;
	}else{
		mensaje->nuevoNombre = string_new();
	}

	return mensaje;
}

int recibirSize(t_socket* pokedexServer){

	char nuevoBuffer[sizeof(int)];
	int size = 0;
	//Recibo buffer del servidor
	if(recv(pokedexServer->descriptor, nuevoBuffer, sizeof(int), 0)>0){
		int sizeAux = 0;
		deserializarIntSize(nuevoBuffer, &size, sizeAux);
		return size;
	}else
		return 0;
}

void recibirBuffer(t_socket* pokedexServer, char* nuevoBuffer, int size){

	//Recibo buffer del servidor

	int sizeRecibido = recv(pokedexServer->descriptor, nuevoBuffer, size, 0); //600 recibo 400

	int sizeNecesitado = size; //600
	int offset = 0;

	while(sizeRecibido < sizeNecesitado){ //400 < 600 -- 50< 200 -- 100<150

		sizeNecesitado -= sizeRecibido; // 200 -- 150 -- 50

		offset += sizeRecibido; //400 -- 450

		sizeRecibido =  recv(pokedexServer->descriptor, nuevoBuffer + offset, sizeNecesitado, 0); // 200 recibo 50 // 150 recibo 100 --50

	}

}

void enviarBuffer(t_socket* pokedexServer, char* buf, int size){

	send(pokedexServer->descriptor, buf, size, 0);

}

void serializarListaOsada(char *buffer, t_list *listaOsada, int sizeAux){

	int size = sizeAux;

	void closure(void *value){

		osada_file *file = (osada_file *) value;
		size = serializarOsadaSize(buffer, file, size);
	}

	list_iterate(listaOsada, closure);
}

t_list *deserializarListaOsada(char *buffer, int cantidadFiles, int sizeAux){

	t_list *list = list_create();
	int size = sizeAux;

	int i= 0;
	for(i=0;i<cantidadFiles;i++){

		if(i == 841){
			printf("llegue");
		}
		osada_file *file = malloc(sizeof(osada_file));
		size = deserializarOsadaSize(buffer, file, size);
		list_add(list, file);
	}

	return list;
}


int serializarOsadaSize(char *ptrBuffer,osada_file *osadaFile, int size){

	memcpy(ptrBuffer + size, &osadaFile->state, sizeof(osada_file_state));
	size += sizeof(osada_file_state);
	memcpy(ptrBuffer + size, osadaFile->fname, sizeof(char) * OSADA_FILENAME_LENGTH);
	size += sizeof(char) * OSADA_FILENAME_LENGTH;
	int fileSize = (int) osadaFile->file_size;
	size = serializarIntSize(ptrBuffer, &(fileSize), size);
	int lastmod = (int) osadaFile->lastmod;
	size = serializarIntSize(ptrBuffer, &(lastmod), size);

	return size;
}

osada_file *deserializarOsada(t_socket_buffer *ptrBuffer){

	osada_file *file = malloc(sizeof(osada_file));
	osada_file_state *state = deserializarOsadaState(ptrBuffer);
	file->state = *state;
	free(state);
	char name[OSADA_FILENAME_LENGTH];
	deserializarOsadaName(ptrBuffer, name);
	memcpy(file->fname, name, OSADA_FILENAME_LENGTH);
	int size = 0;
	deserializarInt(ptrBuffer, &(size));
	file->file_size = (uint32_t) size;
	int lastmod = 0;
	deserializarInt(ptrBuffer, &(lastmod));
	file->lastmod = (uint32_t) lastmod;
	file->parent_directory = 0;
	file->first_block = 0;

	return file;
}

int deserializarOsadaSize(char *ptrBuffer, osada_file *file, int size){

	memcpy(&file->state, ptrBuffer + size, sizeof(osada_file_state));
	size += sizeof(osada_file_state);
	memcpy(file->fname, ptrBuffer + size, OSADA_FILENAME_LENGTH);
	size += OSADA_FILENAME_LENGTH;
	int fileSize = 0;
	memcpy(&fileSize, ptrBuffer + size, sizeof(int));
	file->file_size = (uint32_t) fileSize;
	size += sizeof(int);
	int lastmod = 0;
	memcpy(&lastmod, ptrBuffer + size, sizeof(int));
	file->lastmod = (uint32_t) lastmod;
	size += sizeof(int);
	file->parent_directory = 0;
	file->first_block = 0;

	return size;
}

void serializarOsadaState(t_socket_buffer *buffer, osada_file_state *state){
	int size = buffer->size;
	memcpy(buffer->data + size, state, sizeof(osada_file_state));
	buffer->size = buffer->size + sizeof(osada_file_state);
}

osada_file_state *deserializarOsadaState(t_socket_buffer *buffer){

	osada_file_state *state = malloc(sizeof(osada_file_state));
	int size = buffer->size;
	memcpy(state, buffer->data + size, sizeof(osada_file_state));
	buffer->size = buffer->size + sizeof(osada_file_state);
	return state;
}

void serializarOsadaName(t_socket_buffer *buffer, char *name){
	int size = buffer->size;
	memcpy(buffer->data + size, name, sizeof(char) * OSADA_FILENAME_LENGTH);
	buffer->size = buffer->size + (sizeof(char) * OSADA_FILENAME_LENGTH);
}

void deserializarOsadaName(t_socket_buffer *buffer, char *name){
	int size = buffer->size;
	memcpy(name, buffer->data + size, sizeof(char) * OSADA_FILENAME_LENGTH);
	buffer->size = buffer->size + (sizeof(char) * OSADA_FILENAME_LENGTH);
}


int serializarLongSize(char *buffer, long* dato, int size){
	memcpy(buffer + size, dato, sizeof(long));
	return size + sizeof(long);
}

int deserializarLongSize(char *buffer, long *dato, int size){
	memcpy(dato, buffer + size, sizeof(long));
	return size + sizeof(long);
}
