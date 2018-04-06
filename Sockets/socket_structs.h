/*
 * socket_structs.h
 *
 *  Created on: 6/9/2016
 *      Author: Facundo Castellano
 */

#ifndef SOCKET_STRUCTS_H_
#define SOCKET_STRUCTS_H_
#define BUFF_SIZE 200
#define SIZE_OF_INFO_FILE 26
#include "tipos.h"
#include <stdio.h>
#include <commons/collections/list.h>
#include <commons/collections/dictionary.h>
#include <pkmn/factory.h>
#include <time.h>
#include "osada.h"

typedef struct {
	int descriptor;
	struct sockaddr_in *ptrAddress;
} t_socket;

typedef struct {
	t_socket *ptrSocket;
	t_socket *ptrSocketServer;
} t_socket_client;

typedef struct {
	char data[BUFF_SIZE];
	int size;
} t_socket_buffer;

typedef struct {
	char simbolo;
	int	vidas;
	int reintentos;
	int accion;
	int posicionX;
	int posicionY;
	int movimiento;
	char pokemon;
	char *nombre;
} t_msj_entrenador;

typedef struct{
	t_socket *socket;
	t_msj_entrenador *msjEntrenador;
	t_dictionary *pokemonsAtrapados;
	time_t tiempoDeIngreso;
	char simbolo;
	int quantum;
} t_info_entrenador;

typedef struct {
	int tiempoChequeoDeadlock;
	int batalla;
	char* algoritmo;
	int quantum;
	int retardo;
	char* ip;
	int puerto;
} t_msj_mapa;

typedef struct{
	int nivel;
	char *nombreArchivo;
	char *nombrePokemon;
}t_info_pokemon;

typedef struct{
	char* path;
	char* nuevoNombre;
	int accion;
	int off;
	int size;
	long time;
}t_msj_pokedex;

typedef struct{
	t_info_entrenador *entrenador;
	t_pokemon *pokemonElegido;
} t_info_batalla_pokemon;

typedef struct{
	int res;
	t_list *list;
}t_msj_readdir;

typedef struct{
	int res;
	osada_file *file;
}t_msj_getattr;

typedef struct{
	int res;
	int sizeRead;
}t_msj_read;

#endif /* SOCKET_STRUCTS_H_ */
