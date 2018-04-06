/*
 * entrenador.h
 *
 *  Created on: 4/9/2016
 *      Author: utnso
 */

#ifndef ENTRENADOR_H_
#define ENTRENADOR_H_

#include <commons/config.h>
#include "estructuras.h"

void cargarEntrenador();
void recargarEntrenador(t_entrenador *entrenador);
int gestionarEntrenador(t_socket *cliente, t_info_mapa *mapa);

void cargarPokemons(t_info_mapa *mapa);
t_queue *crearColaPokemon(char **pokemones);

t_queue *calcularColaMovimientos(t_entrenador *entrenador, t_msj_entrenador *mensajeEntrenador);
t_queue *obtenerColaPosY(int posEntrenador, int posPokenest);
t_queue *obtenerColaPosX(int posEntrenador, int posPokenest);
t_queue *intercarlarColas(t_queue *colaPosY, t_queue *colaPosX);

t_info_pokemon *getMejorPokemon(t_list *pokemones);

void cargarInfoMapa(t_info_conexion* conexion, char* mapa);

int iniciarMapa(t_info_mapa *mapa,t_msj_entrenador *msjEntrenador);
void initEntrenador();

void guardarEnLoDeBill(char *nombreMapa, char idPokemon, t_info_pokemon *pokemon);
char *obtenerDirectorioPokemon(char *nombreMapa, char *idPokemon);
void copiarEnDirectorio(char *pathFrom, char *pathTo);
char *modificarEspaciosEntreDirectorios(char *path);
void obtenerMedalla(char *nombreMapa);

void borrarPokemons();
void freeEntrenador(t_entrenador *entrenador);

#endif /* ENTRENADOR_H_ */
