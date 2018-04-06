/*
 * deadlock.h
 *
 *  Created on: 4/10/2016
 *      Author: Facundo Castellano
 */

#ifndef DEADLOCK_H_
#define DEADLOCK_H_

#include <commons/log.h>
#include <stdbool.h>
#include <time.h>
#include <commons/config.h>
#include <commons/string.h>
#include "estructuras.h"
#include "queue_utils.h"
#include "pokemon_utils.h"
#include "socketes_servidor.h"
#include "interfaz_grafica.h"
#include <pkmn/battle.h>
#include <pkmn/factory.h>
#include "planificador.h"

void *hiloDeadlock();

t_dictionary *crearVectorDisponibles();
t_dictionary *crearMatrizAsignacion(t_queue *colaBloqueados);
t_dictionary *crearMatrizNecesidad(t_queue *colaBloqueados);

t_dictionary *obtenerRecursosAsignados(t_dictionary *pokemons);
t_dictionary *obtenerRecursosNecesitados(char *pokemon);

void calcularMatriz(t_dictionary *recursosDisponibles, t_dictionary *matrizAsignacion, t_dictionary *matrizNecesidad);
bool sufreInanicion(char *entrenador, t_dictionary *matrizAsignacion, t_dictionary *matrizNecesidad);

t_list *getDeadlock(t_dictionary *matrizCalculada, t_queue *colaBloqueados);
void liberarRecursos(t_info_entrenador *entrenador);
bool puedeEjecutar(t_dictionary *disponibles, t_dictionary *necesitados);

void aumentarDisponible(t_dictionary *disponibles, t_dictionary *asignados);

void batallaPokemon(t_list *entrenadores);
t_info_entrenador *ejecutarBatalla(t_list *peleadores);
t_queue *obtenerColaPokemon(t_list *peleadores);
t_pokemon *obtenerPerdedor(t_queue *colaPokemon);

void entregarPokemons (t_list *peleadores);
t_info_entrenador *obtenerEntrenadorPerdedor(t_list *peleadores, t_pokemon *pokemon);
t_list *prepararPeleadores(t_list *entrenadores, bool *hayUnDesconectado);
void liberarPokemons(t_queue *colaPokenest, t_queue *colaEntrenador);

#endif /* DEADLOCK_H_ */
