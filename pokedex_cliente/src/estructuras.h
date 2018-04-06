/*
 * estructuras.h
 *
 *  Created on: 1/10/2016
 *      Author: utnso
 */

#ifndef ESTRUCTURAS_H_
#define ESTRUCTURAS_H_

#include <commons/collections/list.h>

typedef struct {
    char* nombre;
    char* simbolo;
    t_list* hojaDeViaje;
    int vidas;
	int reintentos;
}t_entrenador;

typedef struct{
	int tiempoChequeoDeadlock;
	int batalla;
	char* algoritmo;
	int quantum;
	int retardo;
	char* ip;
	int puerto;

}t_mapa;

#endif /* ESTRUCTURAS_H_ */
