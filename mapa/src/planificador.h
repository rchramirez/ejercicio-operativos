/*
 * planificador.h
 *
 *  Created on: 22/9/2016
 *      Author: utnso
 */

#ifndef PLANIFICADOR_H_
#define PLANIFICADOR_H_

#include <commons/log.h>
#include <stdbool.h>
#include <commons/collections/queue.h>
#include <commons/config.h>
#include <commons/string.h>
#include <signal.h>
#include "estructuras.h"
#include "socketes_servidor.h"
#include "queue_utils.h"
#include "pokemon_utils.h"
#include "interfaz_grafica.h"
#include "deadlock.h"

t_log* log_planificador;
t_log* log_block;
t_log* log_deadlock;

void *hiloPlanificador();
void *hiloBlock();

t_info_entrenador *quienSigue(t_mapa *planificador);
t_info_entrenador *getReadySRDF();
t_info_entrenador *getReadyRR();

bool isTurno(t_info_entrenador *entrenador, t_mapa *mapa);
int getDistancia(t_info_entrenador *entrenador);

void recargarPlanificador(t_mapa *planificador);

t_info_entrenador *getEntrenadorMasCercano();
void atenderAlPrimeroSinPokenest();

bool sePuedeAtenderUnEntrenador();

#endif /* PLANIFICADOR_H_ */


