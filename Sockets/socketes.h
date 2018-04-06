/*
 * socketes.h
 *
 *  Created on: 04/09/2016
 *      Author: Facundo Borda
 */

#ifndef SRC_SOCKETES_H_
#define SRC_SOCKETES_H_

#include "socket_structs.h"

#define MAX_CONEXIONES 50 // Hay que ver si es necesario

//Mensajes entre el mapa y el entrenador
#define NUEVO_ENTRENADOR	1
#define MUERTE_ENTRENADOR	2
#define BATALLA_POKEMON		3
#define NUEVO_TURNO			4
#define TURNO_FINALIZADO	5
#define UBICACION_POKENEST	6
#define CAPTURAR_POKEMON	7
#define MOVERSE				8
#define OBJETIVO_CUMPLIDO	9
#define OBJETIVO_PERDIDO	11
#define MODIFICAR_VIDA		12
#define REINICIAR			13
#define UBICACION_POKENEST_OTORGADA	16


//Mensajes entre el pokedex servidor y el pokedex cliente
#define RENAME				41
#define READ 				42
#define READDIR 			43
#define GETATTR  			44
#define MKDIR				45
#define WRITE				46
#define OK					47
#define ERROR				48
#define RM					49
#define RMDIR				50
#define CREATE				51
#define TRUNCATE			52
#define UTIMENS				53
//FUNCIONES DE MANEJO DE SOCKETS
//=================================================================

Boolean socketDestroy(t_socket *ptrSocket);
void closeSocket(t_socket *cliente);
//=================================================================

void enviarMsjEntrenador(t_socket *descriptor, t_msj_entrenador *msjEntrenador);
t_msj_entrenador *recibirMsjEntrenador(t_socket *descriptor);

void enviarMsjPokedex(t_socket *descriptor, t_msj_pokedex *mensaje);
void enviarMsjReaddir(t_socket *descriptor, t_list *files, int res);
void enviarMsjGetattr(t_socket *descriptor, osada_file *file, int res);
void enviarMsjRead(t_socket *descriptor, char *buf, int res, int size);

t_msj_pokedex *recibirMsjPokedex(t_socket *descriptor);
t_msj_readdir *recibirMsjReaddir(t_socket *descriptor);
t_msj_getattr *recibirMsjGetattr(t_socket *descriptor);
t_msj_read *recibirMsjRead(t_socket *descriptor, char *buf);

void serializarEntrenador(t_socket *descriptor, t_msj_entrenador *msjEntrenador);
t_msj_entrenador *deserializarEntrenador(char *buffer);

void enviarPokemon(t_socket *descriptor, t_info_pokemon *pokemon);
t_info_pokemon *recibirPokemon(t_socket *descriptor);

void serializarPokemon(t_socket *descriptor, t_info_pokemon *pokemon);
t_info_pokemon *deserializarPokemon(char *buffer);

int serializarCharSize(char *buffer, char* dato, int size);
int deserializarCharSize(char *buffer, char* dato, int size);
void deserializarInt(t_socket_buffer *buffer, int* dato);
int serializarIntSize(char *buffer, int* dato, int size);
int deserializarIntSize(char *buffer, int *dato, int size);
int serializarLongSize(char *buffer, long* dato, int size);
int deserializarLongSize(char *buffer, long *dato, int size);

void serializarMensajePokedex(t_socket *descriptor,t_msj_pokedex *mensaje);
t_msj_pokedex *deserializarMensajePokedex(char *buffer);

void recibirBuffer(t_socket* pokedexServer, char* nuevoBuffer, int size);
void enviarBuffer(t_socket* pokedexServer, char* buf, int size);

int recibirSize(t_socket* pokedexServer);

void serializarListaOsada(char *buffer, t_list *listaOsada, int sizeAux);
int serializarOsadaSize(char *ptrBuffer,osada_file *osadaFile, int size);
t_list *deserializarListaOsada(char *buffer, int cantidadFiles, int sizeAux);
osada_file *deserializarOsada(t_socket_buffer *ptrBuffer);
int deserializarOsadaSize(char *ptrBuffer, osada_file *file, int size);

void serializarOsadaState(t_socket_buffer *buffer, osada_file_state *state);
osada_file_state *deserializarOsadaState(t_socket_buffer *buffer);
void serializarOsadaName(t_socket_buffer *buffer, char *name);
void deserializarOsadaName(t_socket_buffer *buffer, char *name);

#endif /* SRC_SOCKETES_H_ */
