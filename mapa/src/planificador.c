/*
 * planificador.c
 *
 *  Created on: 17/9/2016
 *      Author: utnso
 */

#include <stdlib.h>
#include <stdio.h>
#include "planificador.h"

void *hiloPlanificador() {

	char *nombreMapa = string_from_format("%s", NOMBRE_MAPA);
	struct timespec tim2;
	tim2.tv_sec = 0;

	char *nombreLog = string_from_format("%s-PLAN", NOMBRE_MAPA);
	log_planificador = log_create("mapa.log", nombreLog, FALSE, LOG_LEVEL_TRACE);
	free(nombreLog);
	//Comienzo del hilo planificador
	log_trace(log_planificador, "Inicie la planificacion");

	log_trace(log_planificador, "Creo el hilo Block");
	pthread_create (&idHiloBlock, NULL, (void*) hiloBlock, NULL);
	log_trace(log_planificador, "Creo el hilo Deadlock");
	//Creo el hilo deadlock
	pthread_create (&idHiloDeadlock, NULL, (void*) hiloDeadlock, NULL);

	t_info_entrenador *entrenador = NULL;
	t_msj_entrenador* msjEntrenador = NULL;
	bool sigueTurno = false;
	bool isBlock = false;

	while(1){

		log_trace(log_planificador, "Quien sigue?");
		entrenador = quienSigue(mapa);
		if(entrenador == NULL){
			log_trace(log_planificador, "No hay nadie en Ready para planificar");
			//Ya no hay nadie en la cola de listos, espero alguno.
			pthread_mutex_lock(&mutexGestionReady);
			pthread_cond_wait(&condPlanificador, &mutexGestionReady);
			pthread_mutex_unlock(&mutexGestionReady);
			log_trace(log_planificador, "Ya hay alguien en Ready para planificar!");
		}
		sigueTurno = true;
		isBlock = false;
		while(sigueTurno){

			entrenador->msjEntrenador = recibirMsjEntrenador(entrenador->socket);
			msjEntrenador = entrenador->msjEntrenador;

			if(msjEntrenador == NULL){
				entrenador = NULL;
				sigueTurno = false;
				mostrarPantalla(nombreMapa);
				break;
			}

			char accion = (char) msjEntrenador->accion;

			switch(accion){

				case MOVERSE: {

					log_trace(log_planificador, "%s me pide moverse", msjEntrenador->nombre);
					moverEntrenador(msjEntrenador->simbolo, msjEntrenador->movimiento);
					mostrarPantalla(nombreMapa);

					break;
				}
				case CAPTURAR_POKEMON: {

					log_trace(log_planificador, "%s me pide capturar un %c", msjEntrenador->nombre, msjEntrenador->pokemon);
					log_trace(log_mapa, "Bloqueo a %s", msjEntrenador->nombre);
					//Bloqueo al entrenador
					addToBlock(entrenador);

					if(queue_size(colaBlock) == 1){
						log_trace(log_planificador, "Desbloqueo block");
						pthread_mutex_lock(&mutexGestionHilos);
						pthread_cond_signal(&condBlockEmpty);
						pthread_mutex_unlock(&mutexGestionHilos);

					}

					//Despierto el hilo para que pueda ver si tiene disponible el pokemon que pido
					pthread_mutex_lock(&mutexGestionBlock);
					pthread_cond_signal(&condBlock);
					pthread_mutex_unlock(&mutexGestionBlock);

					entrenador = NULL;
					sigueTurno = false;
					isBlock = true;
					break;
				}
				case OBJETIVO_CUMPLIDO: {

					log_trace(log_mapa, "%s cumplio su objetivo!!", msjEntrenador->nombre);
					entrenador = NULL;
					sigueTurno = false;
					mostrarPantalla(nombreMapa);
					break;
				}
			}

			if(sigueTurno)
				sigueTurno = isTurno(entrenador, mapa);

			// Es falso cuando quiere atrapar un pokemon o se le terminaron los q
			if(sigueTurno){
				log_trace(log_mapa, "Sigue el turno de %s, QUANTUM UTILIZADO = %d", msjEntrenador->nombre, entrenador->quantum);
				enviarMsjEntrenador(entrenador->socket, msjEntrenador);
				free(msjEntrenador->nombre);
				free(msjEntrenador);
			}

			tim2.tv_sec = mapa->retardo / 1000;
			tim2.tv_nsec = (mapa->retardo % 1000) * 1000000;
			nanosleep(&tim2, NULL);
		}
		if(!isBlock && entrenador != NULL){
			log_trace(log_mapa, "%s vuelve a Ready", msjEntrenador->nombre);
			addToReady(entrenador);
		}
	}
	pthread_exit(NULL);
}

/*
 * Gestion de la cola bloqueados. Se encarga de enviar los pokemons a los entrenadores que le piden (si quedan pokemons)
 */
void *hiloBlock(){


	char *nombreLog = string_from_format("%s-BLOCK", NOMBRE_MAPA);
	log_block = log_create("mapa.log", nombreLog, FALSE, LOG_LEVEL_TRACE);
	free(nombreLog);

	log_trace(log_block, "Se inicio el hilo block");

	while(1){

		//Si no hay ningun entrenador en block me quedo esperando
		while(queue_is_empty(colaBlock)){
			pthread_mutex_lock(&mutexGestionHilos);
			log_trace(log_block, "No hay entrenadores bloqueados");
			pthread_cond_wait(&condBlockEmpty, &mutexGestionHilos);
			log_trace(log_block, "Se encontro bloqueados");
			pthread_mutex_unlock(&mutexGestionHilos);
		}

		//Evito que se pisen el hilo bloqueado y el deadlock
		pthread_mutex_lock(&mutexBlockDeadlock);

		//Retiro un entrenador de la cola de bloqueados
		t_info_entrenador *entrenador = getBlock();

		log_trace(log_block, "Obtengo un pokemon %c para %s", entrenador->msjEntrenador->pokemon, entrenador->msjEntrenador->nombre);

		char *idPokemon = string_from_format("%c", entrenador->msjEntrenador->pokemon);
		t_info_pokemon *pokemon = obtenerPokemon(idPokemon);
		free(idPokemon);

		if(pokemon == NULL){

			log_trace(log_block, "No hay pokemon %c para %s", entrenador->msjEntrenador->pokemon, entrenador->msjEntrenador->nombre);
			addToBlock(entrenador);
			//Me fijo si hay algun entrenador que pueda entregarle un pokemon. Si hay, sigo preguntando, si no, duermo el hilo
			if(!sePuedeAtenderUnEntrenador()){
				pthread_mutex_unlock(&mutexBlockDeadlock);
				pthread_mutex_lock(&mutexGestionBlock);
				pthread_cond_wait(&condBlock, &mutexGestionBlock);
				pthread_mutex_unlock(&mutexGestionBlock);
			}
			pthread_mutex_unlock(&mutexBlockDeadlock);

		}else {
			log_trace(log_block, "Envio un %s para %s", pokemon->nombrePokemon, entrenador->msjEntrenador->nombre);
			//Puedo dejar que el hilo deadlock ejecute de modo seguro
			//Agrego el pokemon a la lista de atrapados del entrenador
			agregarPokemon(entrenador, pokemon);
			//Envio dos mensajes: uno con el movimiento (ya se sabe que es CAPTURAR_POKEMON)
			enviarMsjEntrenador(entrenador->socket, entrenador->msjEntrenador);
			enviarPokemon(entrenador->socket, pokemon);
			//Espero un nuevo mensaje con el pedido de ubicacion pokenest (o de objetivo cumplido)
			entrenador->msjEntrenador = recibirMsjEntrenador(entrenador->socket);
			pthread_mutex_unlock(&mutexBlockDeadlock);
			//Si es null o el mensaje es objetivo cumplido, lo borro del mapa

			if(entrenador->msjEntrenador == NULL || entrenador->msjEntrenador->accion == OBJETIVO_CUMPLIDO){

				if(entrenador->msjEntrenador != NULL){
					log_trace(log_mapa, "%s cumplio su objetivo!!", entrenador->msjEntrenador->nombre);
					free(entrenador->msjEntrenador->nombre);
					free(entrenador->msjEntrenador);
				}
				//free(entrenador);
				entrenador = NULL;
				char *nombreMapa = string_from_format("%s", NOMBRE_MAPA);
				mostrarPantalla(nombreMapa);
				free(nombreMapa);
			}else{

				//Lo agrego a la cola de listos
				log_trace(log_block, "Agrego a %s en la cola Ready", entrenador->msjEntrenador->nombre);
				addToReady(entrenador);
				//Si la cola tiene solo un entrenador, el hilo planificador no esta levantado
				if(queue_size(colaReady) == 1){
					//El planificador tendria que estar esperando a que pase esto, lo desbloqueo
					pthread_mutex_lock(&mutexGestionReady);
					pthread_cond_signal(&condPlanificador);
					pthread_mutex_unlock(&mutexGestionReady);
				}
			}

		}
	}
	pthread_exit(NULL);
}

/**
 * FUNCIONES DE PLANIFICACION
 */

t_info_entrenador *quienSigue(t_mapa *planificador){

	int algoritmo = planificador->algoritmo;

	//Si no hay ningun entrenador en ready me quedo esperando
	while(queue_is_empty(colaReady)){
		log_trace(log_planificador, "No hay nadie en Ready para planificar");
		pthread_mutex_lock(&mutexGestionReady);
		pthread_cond_wait(&condPlanificador, &mutexGestionReady);
		pthread_mutex_unlock(&mutexGestionReady);
		log_trace(log_planificador, "Ya hay alguien en Ready para planificar!");
	}

	t_info_entrenador *entrenador = NULL;

	//Busco el proximo entrenador (Dependiendo del algoritmo)
	if(algoritmo == ALGORITMO_SRDF)
		entrenador = getReadySRDF();
	else
		entrenador = getReadyRR();

	return entrenador;


}

t_info_entrenador *getReadySRDF(){

	log_trace(log_planificador, "Estoy con el algoritmo SRDF");
	log_trace(log_planificador, "Atiendo al primer entrenador que me pida la pokenest.");
	atenderAlPrimeroSinPokenest();

	log_trace(log_planificador, "Busco al entrenador mas cercano a su objetivo.");
	return getEntrenadorMasCercano();
}

t_info_entrenador *getReadyRR(){

	log_trace(log_planificador, "Estoy con el algoritmo RR");

	t_info_entrenador *entrenador = getReady();

	if(entrenador->msjEntrenador->accion == UBICACION_POKENEST){
		log_trace(log_planificador, "Es el turno de %s", entrenador->msjEntrenador->nombre);
		log_trace(log_planificador, "%s me pidio la posicion de la pokenest %c", entrenador->msjEntrenador->nombre, entrenador->msjEntrenador->pokemon);
		//Si el entrenador pide la ubicacion de la pokenest, se la envio y le sumo un quantum
		char *pokemon = string_from_format("%c", entrenador->msjEntrenador->pokemon);
		t_posicion *pokenest = (t_posicion *)getUbicacionPokenest(pokemon);
		//Busca la ubicacion de la pokenest del pokemon que le llega y la devuelve
		//Estos datos siempre persisten, para el caso de que el algoritmo sea SRDF
		entrenador->msjEntrenador->posicionX = pokenest->posicionX;
		entrenador->msjEntrenador->posicionY = pokenest->posicionY;

		enviarMsjEntrenador(entrenador->socket, entrenador->msjEntrenador);

		log_trace(log_planificador, "Le envie la posicion de la pokenest %c a %s. X = %d, Y = %d",
				entrenador->msjEntrenador->pokemon, entrenador->msjEntrenador->nombre, entrenador->msjEntrenador->posicionX,
				entrenador->msjEntrenador->posicionY);

		free(pokemon);
		free(pokenest);

		entrenador->quantum = entrenador->quantum + 1;
		entrenador->msjEntrenador->accion = UBICACION_POKENEST_OTORGADA;
		//log_debug(log_mapa, "ENVIE POSICION POKENEST");
	}else {
		entrenador->quantum = 0;
		log_trace(log_planificador, "Es el turno de %s", entrenador->msjEntrenador->nombre);
		//Envio el mensaje que no se mando porque termino su turno
		enviarMsjEntrenador(entrenador->socket, entrenador->msjEntrenador);
	}

	return entrenador;
}

bool isTurno(t_info_entrenador *entrenador, t_mapa *mapa){

	//Si es SDRF siempre va a ser su turno hasta que muera o se bloquee
	if(mapa->algoritmo == ALGORITMO_SRDF)
		return true;
	//Si es RR me fijo su quantum.
	else{

		entrenador->quantum = entrenador->quantum + 1;

		return (entrenador->quantum < mapa->quantum);

	}

}

void atenderAlPrimeroSinPokenest(){

	t_info_entrenador *entrenador = NULL;
	bool atendido = false;
	t_queue *colaClonada = clonarCola(colaReady, &mutexColaReady);

	while(!atendido){

		if(queue_is_empty(colaClonada)){
			atendido = true;
		}else{

			entrenador = queue_pop(colaClonada);
			//Si el entrenador pide la ubicacion de la pokenest uso su turno solo para enviarselo
			if(entrenador->msjEntrenador->accion == UBICACION_POKENEST)
			{

				log_trace(log_planificador, "%s me pidio la posicion de la pokenest %c", entrenador->msjEntrenador->nombre, entrenador->msjEntrenador->pokemon);

				char *pokemon = string_from_format("%c", entrenador->msjEntrenador->pokemon);
				t_posicion *pokenest = (t_posicion *)getUbicacionPokenest(pokemon);
				//Busca la ubicacion de la pokenest del pokemon que le llega y la devuelve
				//Estos datos siempre persisten, para el caso de que el algoritmo sea SRDF
				entrenador->msjEntrenador->posicionX = pokenest->posicionX;
				entrenador->msjEntrenador->posicionY = pokenest->posicionY;

				enviarMsjEntrenador(entrenador->socket, entrenador->msjEntrenador);

				log_trace(log_planificador, "Le envie la posicion de la pokenest %c a %s. X = %d, Y = %d",
								entrenador->msjEntrenador->pokemon, entrenador->msjEntrenador->nombre, entrenador->msjEntrenador->posicionX,
								entrenador->msjEntrenador->posicionY);

				entrenador->msjEntrenador->accion = UBICACION_POKENEST_OTORGADA;

				atendido = true;

				//Muevo al entrenador al final de la cola de readys
				moveToEnd(colaReady, &mutexColaReady, entrenador);
				log_trace(log_planificador, "Movi al entrenador al final de la cola");
				free(pokenest);
				free(pokemon);
			}

		}
	}

	log_trace(log_planificador, "Termine de atender al mas cercano");

	queue_destroy(colaClonada);
}

t_info_entrenador *getEntrenadorMasCercano(){

	t_info_entrenador *entrenador = NULL;
	t_info_entrenador *entrenadorMax = NULL;
	int distanciaMax = 0;
	int distancia = 0;

	//Clono la cola de readys
	t_queue *colaClonada = clonarCola(colaReady, &mutexColaReady);

	while(!queue_is_empty(colaClonada)){

		entrenador = queue_pop(colaClonada);
		//Si el entrenador pide una pokenest, no calculo su distancia (ya que no la tiene)
		if(entrenador->msjEntrenador->accion != UBICACION_POKENEST){
			//Obtengo al mejor posicionado
			if(entrenadorMax == NULL){
				entrenadorMax = entrenador;
				distanciaMax = getDistancia(entrenador);
			}else {
				distancia = getDistancia(entrenador);
				//Si la distancia es mas corta, esta mejor posicionado
				if(distancia < distanciaMax){
					entrenadorMax = entrenador;
					distanciaMax = distancia;
				}
			}
		}

	}

	if(entrenadorMax != NULL){
		//Hago pop de la cola de ready retornarlo despues
		popEntrenador(colaReady, &mutexColaReady, entrenadorMax);
		log_trace(log_planificador, "El entrenador mejor posicionado es %s. X = %d, Y = %d", entrenadorMax->msjEntrenador->nombre,
					entrenadorMax->msjEntrenador->posicionX, entrenadorMax->msjEntrenador->posicionY);
	}else
		log_trace(log_planificador, "No hay ningun entrenador");

	queue_destroy(colaClonada);

	return entrenadorMax;
}

/*
 * Esta funcion retorna la diferencia entre la posicion (x+y) de la pokenest y la posicion del entrenador
 */
int getDistancia(t_info_entrenador *entrenador){

	int distanciaEntrenador = getDistanciaEntrenador(entrenador->msjEntrenador->simbolo);
	char *pokemon = string_from_format("%c", entrenador->msjEntrenador->pokemon);
	int distanciaPokenest = getDistanciaPokenest(pokemon);
	free(pokemon);
	return distanciaPokenest - distanciaEntrenador;
}

void recargarPlanificador(t_mapa *mapa){

	char *pathMapa = string_from_format("%sMapas/%s/metadata", PATH_POKEDEX, NOMBRE_MAPA);
	t_config* config = config_create(pathMapa);

	mapa->tiempoChequeoDeadlock = config_get_int_value(config, "TiempoChequeoDeadlock");
	mapa->batalla = config_get_int_value(config, "Batalla");
	mapa->retardo = config_get_int_value(config, "retardo");
	char *algoritmo = config_get_string_value(config, "algoritmo");
	if(string_equals_ignore_case(algoritmo, "RR"))
		mapa->algoritmo = ALGORITMO_RR;
	else
		mapa->algoritmo = ALGORITMO_SRDF;
	config_destroy(config);
	free(pathMapa);

}

bool sePuedeAtenderUnEntrenador(){

	t_queue *colaClonada = clonarCola(colaBlock, &mutexColaBlock);

	while(!queue_is_empty(colaClonada)){

		t_info_entrenador *entrenador = queue_pop(colaClonada);
		char *idPokemon = string_from_format("%c", entrenador->msjEntrenador->pokemon);

		pthread_mutex_lock(&mutexPokenests);
			t_pokenest *pokenest = dictionary_get(pokenests, idPokemon);
			if(pokenest->cantidadPokemons > 0){
				free(idPokemon);
				pthread_mutex_unlock(&mutexPokenests);
				return true;
			}
		pthread_mutex_unlock(&mutexPokenests);

	}

	return false;

}
/*int main(int argc, char **argv) {

	char **cadenas = string_split("asd wqer cv vd wad", " ");
	char *nuevoPath = string_new();
	void iterator(char *value){
		string_append_with_format(&nuevoPath, "%s\\ ", value);
	}
	string_iterate_lines(cadenas, iterator);
	printf("%s", nuevoPath);
}*/
