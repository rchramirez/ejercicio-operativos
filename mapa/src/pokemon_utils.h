/*
 * pokemon_utils.h
 *
 *  Created on: 5/10/2016
 *      Author: Facundo Castellano
 */

#ifndef POKEMON_UTILS_H_
#define POKEMON_UTILS_H_

#include <sys/types.h>
#include <dirent.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <stdbool.h>
#include "estructuras.h"
#include "queue_utils.h"
#include "interfaz_grafica.h"
#include "socket_structs.h"

void agregarPokemon(t_info_entrenador *entrenador, t_info_pokemon *pokemon);
t_info_pokemon *obtenerPokemon(char *idPokemon);

#endif /* POKEMON_UTILS_H_ */
