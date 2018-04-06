/*
 * interfaz_grafica.h
 *
 *  Created on: 20/9/2016
 *      Author: utnso
 */

#ifndef INTERFAZ_GRAFICA_H_
#define INTERFAZ_GRAFICA_H_

#include <stdlib.h>
#include "tad_items.h"
#include "estructuras.h"
#include "queue_utils.h"
#include <commons/collections/list.h>

#define MOVE_UP 25
#define MOVE_DOWN 26
#define MOVE_RIGHT 27
#define MOVE_LEFT 28

void mostrarPantalla(char* mapa);
void agregarEntrenador(char idEntrenador);
void borrarEntrenador(char idEntrenador);
void moverEntrenador(char idEntrenador, int movimiento);
int  getDistanciaEntrenador(char idEntrenador);
void crearPokenest(char* key, void* value);
void ponerEnPokenest(char *idPokemon, int cantidad);
void sacarDePokenest(char *idPokemon);
void agregarPokenests();
t_posicion *getUbicacionPokenest(char *pokemon);
int getDistanciaPokenest(char *pokemon);

#endif /* INTERFAZ_GRAFICA_H_ */

