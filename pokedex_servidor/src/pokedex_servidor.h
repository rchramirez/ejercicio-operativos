/*
 * pokedex_servidor.h
 *
 *  Created on: 18/10/2016
 *      Author: utnso
 */

#ifndef POKEDEX_SERVIDOR_H_
#define POKEDEX_SERVIDOR_H_
#include <pthread.h>
#include "socketes_servidor.h"
#include "osada_utils.h"
#include <errno.h>

pthread_t idHiloConexiones;

void *hiloConexiones(t_socket* cliente);

void poke_read(t_socket* cliente, t_msj_pokedex* mensaje);
void poke_readdir(t_socket* cliente, t_msj_pokedex* mensaje);
void poke_getattr(t_socket* cliente, t_msj_pokedex* mensaje);
void poke_mkdir(t_socket* cliente, t_msj_pokedex* mensaje);
void poke_write(t_socket* cliente, t_msj_pokedex* mensaje);
void poke_rename(t_socket* cliente, t_msj_pokedex* mensaje);
void poke_rm(t_socket* cliente, t_msj_pokedex* mensaje);
void poke_rmdir(t_socket* cliente, t_msj_pokedex* mensaje);
void poke_create(t_socket* cliente, t_msj_pokedex* mensaje);
void poke_truncate(t_socket* cliente, t_msj_pokedex* mensaje);
void poke_utimens(t_socket* cliente, t_msj_pokedex* mensaje);

#endif /* POKEDEX_SERVIDOR_H_ */
