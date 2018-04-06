/*
 * queueUtils.h
 *
 *  Created on: 5/10/2016
 *      Author: Facundo Castellano
 */

#ifndef QUEUE_UTILS_H_
#define QUEUE_UTILS_H_

#include <commons/log.h>
#include "estructuras.h"
#include "socket_structs.h"

void crearListasColas();
void matarListasColas();

void addToReady(t_info_entrenador* entrenador);
void addToBlock(t_info_entrenador* entrenador);

t_info_entrenador *getReady();
t_info_entrenador *getBlock();
int getSizeReady();
int getSizeBlock();

t_list *queueToList(t_queue *cola);

t_queue *clonarCola(t_queue *cola, pthread_mutex_t *mutexDeCola);

void popEntrenador(t_queue *cola, pthread_mutex_t *mutexDeCola, t_info_entrenador *entrenador);
t_info_entrenador *getEntrenador(t_queue *cola, char simbolo);

void moveToEnd(t_queue *cola, pthread_mutex_t *mutexDeCola, t_info_entrenador *entrenador);

void logColas();

#endif /* QUEUE_UTILS_H_ */
