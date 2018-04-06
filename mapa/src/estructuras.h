/*
 * estructuras.h
 *
 *  Created on: 3/9/2016
 *      Author: FacundoCastellano
 */

#ifndef ESTRUCTURAS_H_
#define ESTRUCTURAS_H_

#include <commons/collections/list.h>
#include <commons/collections/dictionary.h>
#include <commons/collections/queue.h>
#include <commons/log.h>
#include <pthread.h>
#include "socketes_servidor.h"

#define ALGORITMO_RR		95
#define ALGORITMO_SRDF		96


typedef struct{
	int tiempoChequeoDeadlock;
	int batalla;
	int algoritmo;
	int quantum;
	int retardo;
	char* ip;
	int puerto;

}t_mapa;

typedef struct{
	char* tipo;
	int posicionX;
	int posicionY;
	char *identificador;
	int cantidadPokemons;
	t_queue *pokemons;
	char *nombrePokemon;
}t_pokenest;

typedef struct{
	int posicionX;
	int posicionY;
}t_posicion;

typedef struct{
	t_queue *pokemons;
	int cantidad;
}t_recursos;

char NOMBRE_MAPA[128];
char PATH_POKEDEX[128];

t_mapa *mapa;
t_log *log_mapa;

t_dictionary* pokenests;
pthread_mutex_t mutexPokenests;

t_list* guiMapa;
pthread_mutex_t mutexGuiMapa;

t_queue* colaReady;
pthread_mutex_t mutexColaReady;

t_queue* colaBlock;
pthread_mutex_t mutexColaBlock;

pthread_mutex_t mutexGestionReady;
pthread_mutex_t mutexGestionBlock;
pthread_mutex_t mutexGestionHilos;
pthread_mutex_t mutexBlockDeadlock;

pthread_cond_t condPlanificador;
pthread_cond_t condBlock;
pthread_cond_t condBlockEmpty;

pthread_t idHiloPlanificador;
pthread_t idHiloBlock;
pthread_t idHiloDeadlock;

t_socket* servidor;

#endif /* ESTRUCTURAS_H_ */
