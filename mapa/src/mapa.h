/*
 * mapa.h
 *
 *  Created on: 3/9/2016
 *      Author: FacundoCastellano
 */

#ifndef MAPA_H_
#define MAPA_H_
#include "estructuras.h"
#include "socketes_servidor.h"

void cargarMapa();
void recorrerPokenests(char *path);
int contabilizarPokemons(char* path);
void cargarPokenest(char* path, char *nombrePokemon);
void obtenerPokemons(char *path, t_queue *pokemons, char *nombrePokemon);
void cargarGui();
void cleanMapa();
void limpiarPokenests();

#endif /* MAPA_H_ */
