/*
 * estructuras.h
 *
 *  Created on: 18/10/2016
 *      Author: utnso
 */

#ifndef ESTRUCTURAS_H_
#define ESTRUCTURAS_H_

#include <pthread.h>

typedef struct{
	char* ip;
	int puerto;
}t_pokedex_srv;

pthread_t idHiloConexiones;
#endif /* ESTRUCTURAS_H_ */
