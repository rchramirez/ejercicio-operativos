/*
 * queueUtils.c
 *
 *  Created on: 5/10/2016
 *      Author: Facundo Castellano
 */

#include "queue_utils.h"

void crearListasColas(){
	colaReady = queue_create();
	pthread_mutex_init (&mutexColaReady, NULL);
	colaBlock = queue_create();
	pthread_mutex_init (&mutexColaBlock, NULL);
}
void matarListasColas() {
	queue_destroy_and_destroy_elements(colaReady, free);
	queue_destroy_and_destroy_elements(colaBlock, free);
}

void addToReady(t_info_entrenador* entrenador) {
	pthread_mutex_lock(&mutexColaReady);
	queue_push(colaReady, entrenador);
	pthread_mutex_unlock(&mutexColaReady);
	logColas();
}

void addToBlock(t_info_entrenador* entrenador) {
	//Pasa el quantum del entrenador a 0
	entrenador->quantum = 0;
	pthread_mutex_lock(&mutexColaBlock);
	queue_push(colaBlock, entrenador);
	pthread_mutex_unlock(&mutexColaBlock);
	logColas();
}

t_info_entrenador *getReady() {
	pthread_mutex_lock(&mutexColaReady);
	t_info_entrenador* entrenador = queue_pop(colaReady);
	pthread_mutex_unlock(&mutexColaReady);
	return entrenador;
}

t_info_entrenador *getBlock() {
	pthread_mutex_lock(&mutexColaBlock);
	t_info_entrenador* entrenador = queue_pop(colaBlock);
	pthread_mutex_unlock(&mutexColaBlock);
	return entrenador;
}

int getSizeReady(){
	pthread_mutex_lock(&mutexColaReady);
	int size = queue_size(colaReady);
	pthread_mutex_unlock(&mutexColaReady);
	return size;
}

int getSizeBlock(){
	pthread_mutex_lock(&mutexColaBlock);
	int size = queue_size(colaBlock);
	pthread_mutex_unlock(&mutexColaBlock);
	return size;
}

t_list *queueToList(t_queue *cola) {

	t_list *lista = list_create();

	while(!queue_is_empty(cola)){

		list_add(lista, queue_pop(cola));
	}

	return lista;
}

t_queue *clonarCola(t_queue *cola, pthread_mutex_t *mutexDeCola){

	t_queue *colaClonada = queue_create();
	if(mutexDeCola != NULL)
		pthread_mutex_lock(mutexDeCola);

	int size = queue_size(cola);
	int var = 0;

	for (var = 0; var < size; ++var) {
		void *data = queue_pop(cola);
		queue_push(colaClonada, data);
		queue_push(cola, data);
	}
	if(mutexDeCola != NULL)
		pthread_mutex_unlock(mutexDeCola);

	return colaClonada;
}

void moveToEnd(t_queue *cola, pthread_mutex_t *mutexDeCola, t_info_entrenador *entrenador){

	popEntrenador(cola, mutexDeCola, entrenador);
	pthread_mutex_lock(mutexDeCola);
	queue_push(cola, entrenador);
	pthread_mutex_unlock(mutexDeCola);
}


t_info_entrenador *getEntrenador(t_queue *cola, char simbolo){

	t_info_entrenador *entrenador;
	bool encontrado = false;

	while(!encontrado){

		entrenador = queue_pop(cola);
		if(entrenador->msjEntrenador->simbolo == simbolo)
			encontrado = true;
		else
			queue_push(cola, entrenador);

	}

	return entrenador;
}

void popEntrenador(t_queue *cola, pthread_mutex_t *mutexDeCola, t_info_entrenador *entrenador){

	int posicion = 0;
	int sizeCola = queue_size(cola);
	t_info_entrenador *entrenadorDeCola = NULL;

	pthread_mutex_lock(mutexDeCola);

	for(posicion = 0; posicion < sizeCola; posicion++){
		entrenadorDeCola = queue_pop(cola);
		if(entrenadorDeCola->msjEntrenador->simbolo != entrenador->msjEntrenador->simbolo){
			queue_push(cola, entrenadorDeCola);
		}
	}

	pthread_mutex_unlock(mutexDeCola);
}

void logColas(){

	t_info_entrenador *entrenador = NULL;

	log_info(log_mapa, "----LOG DE COLAS----");

	t_queue *colaClonadaReady = clonarCola(colaReady, &mutexColaReady);
	log_info(log_mapa, "COLA READY - CANTIDAD = %d", queue_size(colaClonadaReady));
	while(!queue_is_empty(colaClonadaReady)){
		entrenador = queue_pop(colaClonadaReady);
		log_info(log_mapa, "ID entrenador = %c", entrenador->simbolo);
	}
	queue_destroy(colaClonadaReady);

	t_queue *colaClonadaBlock = clonarCola(colaBlock, &mutexColaBlock);
	log_info(log_mapa, "COLA BLOCK - CANTIDAD = %d", queue_size(colaClonadaBlock));
	while(!queue_is_empty(colaClonadaBlock)){
		entrenador = queue_pop(colaClonadaBlock);
		log_info(log_mapa, "ID entrenador = %c", entrenador->simbolo);
	}
	queue_destroy(colaClonadaBlock);
}

