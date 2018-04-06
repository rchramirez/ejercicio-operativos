/*
 * deadlock.c
 *
 *  Created on: 4/10/2016
 *      Author: Facundo Castellano
 */

#include "deadlock.h"

void freeDataDictionary(void *data);

void *hiloDeadlock(){

	struct timespec tim;
	char *logName = string_from_format("%s-DEADLOCK", NOMBRE_MAPA);
	log_deadlock = log_create("mapa.log", logName, FALSE, LOG_LEVEL_TRACE);
	free(logName);
	log_trace(log_deadlock, "Inicie el hilo deadlock");

	while(1){

		if(mapa->batalla && queue_size(colaBlock) > 1){
			log_trace(log_deadlock, "Hay mas de un bloqueado. Verifico deadlock");
			//Lockeo el mutex para que el hilo block no realice ningun cambio mientras arreglo el interbloqueo
			pthread_mutex_lock(&mutexBlockDeadlock);
			t_queue *colaBloqueados = clonarCola(colaBlock, &mutexColaBlock);

			log_trace(log_deadlock, "Se crea vector de disponibles.");
			//Creo el vector de disponibles
			t_dictionary *recursosDisponibles = crearVectorDisponibles();

			log_trace(log_deadlock, "Se crea matriz de asignacion.");
			//Creo la matriz de asignacion de los entrenadores bloqueados
			t_dictionary *matrizAsignacion = crearMatrizAsignacion(colaBloqueados);

			log_trace(log_deadlock, "Se crea matriz de necesidad.");
			//Creo la matriz de necesidad de los entrenadores bloqueados
			t_dictionary *matrizNecesidad = crearMatrizNecesidad(colaBloqueados);

			log_trace(log_deadlock, "Se calcula la matriz.");
			//Modifica la matrizNecesidad
			calcularMatriz(recursosDisponibles, matrizAsignacion, matrizNecesidad);

			log_trace(log_deadlock, "Obtengo los entrenadores en deadlock.");
			t_list *entrenadoresEnDeadlock = getDeadlock(matrizNecesidad, colaBloqueados);

			if(!list_is_empty(entrenadoresEnDeadlock)){
				log_trace(log_deadlock, "Hay batalla pokemon!");
				batallaPokemon(entrenadoresEnDeadlock);
			}else{
				log_trace(log_deadlock, "No hay deadlock!");
			}

			dictionary_destroy_and_destroy_elements(recursosDisponibles, free);
			dictionary_destroy_and_destroy_elements(matrizAsignacion, freeDataDictionary);
			dictionary_destroy_and_destroy_elements(matrizNecesidad, freeDataDictionary);
			queue_destroy(colaBloqueados);

			log_trace(log_deadlock, "Sale deadlock");
			pthread_mutex_unlock(&mutexBlockDeadlock);
		}else
			log_trace(log_deadlock, "No hay deadlock. BATALLA = %d, CANTIDAD BLOCKS = %d", mapa->batalla, queue_size(colaBlock));

		if(!queue_is_empty(colaBlock)){
			pthread_mutex_lock(&mutexGestionBlock);
			pthread_cond_signal(&condBlock);
			pthread_mutex_unlock(&mutexGestionBlock);
		}

		tim.tv_sec = mapa->tiempoChequeoDeadlock / 1000;
		tim.tv_nsec = (mapa->tiempoChequeoDeadlock % 1000) * 1000000;
		nanosleep(&tim, NULL);

	}
}

t_dictionary *crearVectorDisponibles(){

	t_dictionary *recursosDisponibles = dictionary_create();

	pthread_mutex_lock (&mutexPokenests);

	void iterator(char* key, void* value){

		char *idPokemon = string_duplicate(key);

		t_pokenest *pokenest = (t_pokenest *) value;
		t_recursos *recursoDisponible = malloc(sizeof(t_recursos));
		recursoDisponible->cantidad = pokenest->cantidadPokemons;
		log_trace(log_deadlock, "POKENEST = %s, CANTIDAD DISPONIBLE = %d", key, recursoDisponible->cantidad);

		dictionary_put(recursosDisponibles, idPokemon, recursoDisponible);
	}
	log_trace(log_deadlock, "VECTOR DE DISPONIBLES (SOLO SE MUESTRAN LOS DISPONIBLES) = ");
	//Itero la coleccion de pokenests. A partir de ella creo un dictionary con id pokenest como clave y los recursos disponibles como value
	dictionary_iterator(pokenests, iterator);

	pthread_mutex_unlock (&mutexPokenests);

	return recursosDisponibles;

}

t_dictionary *crearMatrizAsignacion(t_queue *colaBloqueados){

	t_dictionary *matrizAsignacion = dictionary_create();
	log_trace(log_deadlock, "MATRIZ ASIGNACION (SOLO SE MUESTRAN LOS ASIGNADOS) = ");
	//Itero los entrenadores bloqueados. Voy agregando como key el id del entrenador y como value un vector con sus recursos asignados (pokemonesAtrapados)
	//Clono la cola para poder hacer pop tranquilo
	t_queue *colaClonada = clonarCola(colaBlock, NULL);

	while(!queue_is_empty(colaClonada)){

		t_info_entrenador *entrenador = queue_pop(colaClonada);
		log_trace(log_deadlock, "ENTRENADOR = %c, POKEMONS ASIGNADOS = ", entrenador->msjEntrenador->simbolo);
		t_dictionary *recursosAsignados = obtenerRecursosAsignados(entrenador->pokemonsAtrapados);
		char *idEntrenador = string_from_format("%c", entrenador->msjEntrenador->simbolo);
		if(!dictionary_is_empty(recursosAsignados)){
			dictionary_put(matrizAsignacion, idEntrenador, recursosAsignados);
		}
	}

	queue_destroy(colaClonada);

	return matrizAsignacion;
}

t_dictionary *obtenerRecursosAsignados(t_dictionary *pokemons){

	t_dictionary *recursosAsignados = dictionary_create();

	//Itero los pokemones.
	void iterator(char* key, void* value){

		t_recursos *cantidadPokemon = (t_recursos *) value;

		//Si no tiene recurso asignado, no lo agrego
		if(cantidadPokemon->cantidad > 0){

			char *idPokemon = string_duplicate(key);
			t_recursos *recursos = malloc(sizeof(t_recursos));
			recursos->cantidad = cantidadPokemon->cantidad;
			recursos->pokemons = cantidadPokemon->pokemons;

			log_trace(log_deadlock, "POKEMON = %s, CANTIDAD DISPONIBLE = %d", idPokemon, recursos->cantidad);
			dictionary_put(recursosAsignados, idPokemon, recursos);
		}
	}

	if(!dictionary_is_empty(pokemons))
		dictionary_iterator(pokemons, iterator);

	return recursosAsignados;

}

t_dictionary *crearMatrizNecesidad(t_queue *colaBloqueados){

	t_dictionary *matrizNecesidad = dictionary_create();

	log_trace(log_deadlock, "MATRIZ NECESIDAD (SOLO SE MUESTRAN LOS NECESITADOS) = ");
	//Clono la cola para poder hacer pop tranquilo
	t_queue *colaClonada = clonarCola(colaBloqueados, NULL);

	while(!queue_is_empty(colaClonada)){

		t_info_entrenador *entrenador = queue_pop(colaClonada);

		log_trace(log_deadlock, "ENTRENADOR = %c, POKEMON NECESITADO = %c", entrenador->msjEntrenador->simbolo, entrenador->msjEntrenador->pokemon);

		char *pokemon = string_from_format("%c", entrenador->msjEntrenador->pokemon);
		t_dictionary *recursosNecesitados = obtenerRecursosNecesitados(pokemon);
		char *idEntrenador = string_from_format("%c", entrenador->msjEntrenador->simbolo);
		dictionary_put(matrizNecesidad, idEntrenador, recursosNecesitados);
	}

	queue_destroy(colaClonada);

	return matrizNecesidad;

}

t_dictionary *obtenerRecursosNecesitados(char *pokemon){

	t_dictionary *recursosNecesitados = dictionary_create();

	t_recursos *recursos = malloc(sizeof(t_recursos));
	recursos->cantidad = 1;

	dictionary_put(recursosNecesitados, pokemon, recursos);

	return recursosNecesitados;

}

void calcularMatriz(t_dictionary *recursosDisponibles, t_dictionary *matrizAsignacion, t_dictionary *matrizNecesidad){

	t_dictionary *recursosNecesitados = NULL;
	t_dictionary *recursosAsignados = NULL;

	void matrizNecesidadIterator(char* key, void* value){

		//Obtengo el vector de necesidad del entrenador
		recursosNecesitados = (t_dictionary *) value;
		//Obtengo los recursos asignados del entrenador
		recursosAsignados = dictionary_get(matrizAsignacion, key);

		//Si no tiene recursos asignados, no esta en un deadlock
		if(recursosAsignados != NULL && !dictionary_is_empty(recursosAsignados)){
			if(puedeEjecutar(recursosDisponibles, recursosNecesitados) || sufreInanicion(key, matrizAsignacion, matrizNecesidad)){

				//Si puedo ejecutar, entonces sumo los recursos de la matriz de asignacion al de disponibles
				//y limpio la matriz de necesidad de ese entrenador
				aumentarDisponible(recursosDisponibles, recursosAsignados);
				dictionary_clean(recursosAsignados);
				dictionary_clean(recursosNecesitados);
			}
		}else{
			//Lo remuevo de la matriz de necesidad
			dictionary_remove(matrizNecesidad, key);
		}
	}

	dictionary_iterator(matrizNecesidad, matrizNecesidadIterator);
}

bool sufreInanicion(char *entrenador, t_dictionary *matrizAsignacion, t_dictionary *matrizNecesidad){

	bool tieneInanicion = true;
	//El entrenador sufre inanicion si nadie necesita lo que el tiene
	t_dictionary *recursosAsignados = dictionary_get(matrizAsignacion, entrenador);

	void recursosAsignadosIterator(char* key, void* value){

		//Itero la matriz de necesidad, viendo los recursos necesitados de cada entrenador
		void matrizNecesidadIterator(char* idEntrenador, void* value){

			t_dictionary *recursosNecesitados = (t_dictionary *) value;
			//Busco si alguien necesita sus asignados (que no sea el)
			if(!string_equals_ignore_case(entrenador, idEntrenador) && dictionary_has_key(recursosNecesitados, key))
				tieneInanicion = false;
		}

		dictionary_iterator(matrizNecesidad, matrizNecesidadIterator);
	}

	dictionary_iterator(recursosAsignados, recursosAsignadosIterator);

	if(tieneInanicion)
		log_trace(log_deadlock, "ENTRENADOR = %s TIENE INANICION", entrenador);
	return tieneInanicion;
}

bool puedeEjecutar(t_dictionary *disponibles, t_dictionary *necesitados){

	bool satisface = true;
	//Si necesitados viene vacio, es porque no necesita ningun recurso
	if(dictionary_is_empty(necesitados))
		return satisface;

	//Puede ejecutar si los recursos necesitados son menores o iguales a los disponibles.
	void iterator(char* key, void* value){
		//Si ya se que un recurso no pudo ser obtenido, no es necesario seguir viendo por otros recursos
		if(satisface){
			//Si necesitados no tiene la key, es porque no necesita ese recurso
			if(dictionary_has_key(necesitados, key)){
				t_recursos *cantidadDisponible = (t_recursos *) value;
				//Obtengo la cantidad necesitada de ese recurso
				t_recursos *cantidadNecesitada = dictionary_get(necesitados, key);
				//Si la cantidad necesitada es mas que la disponible, no puedo satisfacer la totalidad de su vector
				if(cantidadDisponible->cantidad < cantidadNecesitada->cantidad)
					satisface = false;
			}
		}
	}

	dictionary_iterator(disponibles, iterator);
	return satisface;
}

void aumentarDisponible(t_dictionary *disponibles, t_dictionary *asignados){

	void iterator(char* key, void* value){

		if(dictionary_has_key(asignados, key)){
			t_recursos *recursosDisponibles = (t_recursos *) value;
			t_recursos *recursosAsignados = dictionary_get(asignados, key);
			recursosDisponibles->cantidad = recursosDisponibles->cantidad + recursosAsignados->cantidad;
		}
	}

	dictionary_iterator(disponibles, iterator);
}

t_list *getDeadlock(t_dictionary *matrizCalculada, t_queue *colaBloqueados){

	t_list *entrenadoresEnDeadlock = list_create();
	t_dictionary *recursosAsignados;
	t_queue *colaClonada = clonarCola(colaBloqueados, NULL);

	void iterator(char* key, void* value){

			recursosAsignados = (t_dictionary *) value;
			//Si hay recursos asignados, es porque no esta marcado. Entonces lo agrego a la lista de deadlock
			if(!dictionary_is_empty(recursosAsignados))
			{
				//Busco el entrenador a partir de la key (simbolo)
				t_info_entrenador *entrenadorEnDeadlock = getEntrenador(colaClonada, *key);
				log_trace(log_deadlock, "El entrenador %s se encuentra interbloqueado.", entrenadorEnDeadlock->msjEntrenador->nombre);
				list_add(entrenadoresEnDeadlock, entrenadorEnDeadlock);
			}
	}

	dictionary_iterator(matrizCalculada, iterator);
	queue_destroy(colaClonada);

	return entrenadoresEnDeadlock;
}

void liberarRecursos(t_info_entrenador *entrenador){

	t_dictionary *recursosAsignados = entrenador->pokemonsAtrapados;

	void iterator(char*key, void *value){

		t_recursos *recurso = (t_recursos *) value;

		//Busco la pokenest de ese recurso (pokemon) y lo agrego.
		pthread_mutex_lock(&mutexPokenests);

		t_pokenest *pokenest = dictionary_get(pokenests,key);
		pokenest->cantidadPokemons = pokenest->cantidadPokemons + recurso->cantidad;
		ponerEnPokenest(key, recurso->cantidad);
		liberarPokemons(pokenest->pokemons, recurso->pokemons);

		free(recurso);

		pthread_mutex_unlock(&mutexPokenests);
	}

	if(!dictionary_is_empty(recursosAsignados))
		dictionary_iterator(recursosAsignados, iterator);
	dictionary_destroy(recursosAsignados);

}

void liberarPokemons(t_queue *colaPokenest, t_queue *colaEntrenador){

	t_info_pokemon *pokemon = NULL;
	while(!queue_is_empty(colaEntrenador)){
		pokemon = queue_pop(colaEntrenador);
		queue_push(colaPokenest, pokemon);
		log_trace(log_planificador, "Se libero el pokemon %s", pokemon->nombrePokemon);
	}
	queue_destroy(colaEntrenador);
}

void batallaPokemon(t_list *entrenadores){

	bool hayUnDesconectado = false;

	t_list *peleadores = prepararPeleadores(entrenadores, &hayUnDesconectado);

	//Si hubo un desconectado no haria falta una batalla, ya que hay recursos.
	if(hayUnDesconectado){
		log_trace(log_deadlock, "Alguien se desconecto. Lo tomo como perdedor");
		entregarPokemons(peleadores);
	}else{

		log_trace(log_deadlock, "Empieza la pelea!!");
		t_info_entrenador *entrenadorPerdedor = ejecutarBatalla(peleadores);
		log_trace(log_deadlock, "%s perdio :/", entrenadorPerdedor->msjEntrenador->nombre);
		//Borro el personaje del mapa
		/*borrarEntrenador(entrenadorPerdedor->simbolo);*/
		liberarRecursos(entrenadorPerdedor);
		popEntrenador(colaBlock, &mutexColaBlock, entrenadorPerdedor);
		char *nombreMapa = string_from_format("%s", NOMBRE_MAPA);
		mostrarPantalla(nombreMapa);
		free(nombreMapa);
		//Le aviso al perdedor
		log_trace(log_deadlock, "Le aviso a %s que perdio", entrenadorPerdedor->msjEntrenador->nombre);
		entrenadorPerdedor->msjEntrenador->accion = MUERTE_ENTRENADOR;
		enviarMsjEntrenador(entrenadorPerdedor->socket, entrenadorPerdedor->msjEntrenador);

		//Entrego los pokemons a los ganadores
		log_trace(log_deadlock, "Le entrego los pokemons a los ganadores");
		entregarPokemons(peleadores);
	}
}

t_list *prepararPeleadores(t_list *entrenadores, bool *hayUnDesconectado){

	t_list *peleadores = list_create();
	t_info_entrenador *entrenador = NULL;
	t_info_pokemon *pokemon = NULL;
	t_pkmn_factory *factory = create_pkmn_factory();

	bool comparator(void *unEntrenador, void *otroEntrenador){

		t_info_entrenador *sortA = (t_info_entrenador *) unEntrenador;
		t_info_entrenador *sortB = (t_info_entrenador *) otroEntrenador;

		return sortA->tiempoDeIngreso < sortB->tiempoDeIngreso;
	}

	list_sort(entrenadores, comparator);

	//TODO sort entrenadores por timestamp
	void iterator(void *value){

		//Le voy pidiendo a cada entrenador su mejor pokemon
		entrenador = (t_info_entrenador *) value;
		entrenador->msjEntrenador->accion = BATALLA_POKEMON;
		log_trace(log_deadlock, "Le aviso a %s que elija a su mejor pokemon.", entrenador->msjEntrenador->nombre);
		enviarMsjEntrenador(entrenador->socket, entrenador->msjEntrenador);

		//Recibo su pokemon y lo agrego a la lista de peleadores
		pokemon = recibirPokemon(entrenador->socket);
		//Si es NULL es porque el entrenador se desconecto. Lo borro del mapa y libero sus recursos
		//TODO: Lo tomo como que el entrenador perdio la batalla y evito la pelea?.
		if(pokemon == NULL){
			popEntrenador(colaBlock, &mutexColaBlock, entrenador);
			char *nombreMapa = string_from_format("%s", NOMBRE_MAPA);
			mostrarPantalla(nombreMapa);
			free(nombreMapa);
			*hayUnDesconectado = true;
		}else {
			log_trace(log_deadlock, "%s eligio a %s para pelear. Lvl = %d", entrenador->msjEntrenador->nombre,
					pokemon->nombrePokemon, pokemon->nivel);
			//Creo al peleador (entrenador con su pokemon elegido) y lo agrego a los peleadores
			t_info_batalla_pokemon *peleador = malloc(sizeof(t_info_batalla_pokemon));
			peleador->entrenador = entrenador;
			char *nombrePokemon = string_duplicate(pokemon->nombrePokemon);
			t_pokemon *pokemonCreado =  create_pokemon(factory, nombrePokemon, pokemon->nivel);
			peleador->pokemonElegido = pokemonCreado;
			list_add(peleadores, peleador);
			//free(nombrePokemon);
		}
	}

	list_iterate(entrenadores, iterator);
	destroy_pkmn_factory(factory);
	return peleadores;
}

/*
 * Ejecuta la batalla, removiendo de la lista de peleadores al perdedor y retornandolo a parte
 */
t_info_entrenador *ejecutarBatalla(t_list *peleadores){

	t_queue *colaPokemon = obtenerColaPokemon(peleadores);
	t_pokemon *pokemonPerdedor = obtenerPerdedor(colaPokemon);

	return obtenerEntrenadorPerdedor(peleadores, pokemonPerdedor);
}

t_queue *obtenerColaPokemon(t_list *peleadores){

	t_queue *colaPokemon = queue_create();

	void iterator(void *value){
		t_info_batalla_pokemon *batalla = (t_info_batalla_pokemon *) value;
		queue_push(colaPokemon, batalla->pokemonElegido);
	}

	list_iterate(peleadores, iterator);

	return colaPokemon;
}

t_pokemon *obtenerPerdedor(t_queue *colaPokemon){

	t_pokemon *pokemonPerdedor = queue_pop(colaPokemon);
	t_pokemon *pokemonSiguiente = NULL;

	while(!queue_is_empty(colaPokemon)){
		pokemonSiguiente = queue_pop(colaPokemon);
		log_trace(log_deadlock, "%s VS %s", pokemonPerdedor->species, pokemonSiguiente->species);
		pokemonPerdedor = pkmn_battle(pokemonPerdedor,pokemonSiguiente);
		log_trace(log_deadlock, "Perdio %s", pokemonPerdedor->species);
	}

	return pokemonPerdedor;
}


t_info_entrenador *obtenerEntrenadorPerdedor(t_list *peleadores, t_pokemon *pokemon){

	t_info_entrenador *entrenadorPerdedor = NULL;
	int i = 0;
	int size = list_size(peleadores);

	for(i=0; i<size;i++){
		t_info_batalla_pokemon *batalla = list_get(peleadores, i);

		if(string_equals_ignore_case(batalla->pokemonElegido->species, pokemon->species)
				&& batalla->pokemonElegido->level == pokemon->level){
			entrenadorPerdedor = batalla->entrenador;
			list_remove(peleadores, i);
			break;
		}
	}
	return entrenadorPerdedor;
}

void entregarPokemons (t_list *peleadores){

	//Le entrego los pokemons a los entrenadores que siguen conectados (los que estan en la lista peleadores)
	void iterator(void *value){
		t_info_batalla_pokemon *batalla = (t_info_batalla_pokemon *) value;

		char *idPokemon = string_from_format("%c", batalla->entrenador->msjEntrenador->pokemon);
		log_trace(log_deadlock, "Obtengo el pokemon %s", idPokemon);
		t_info_pokemon *pokemon = obtenerPokemon(idPokemon);
		//TODO: No se si podria pasar que vuelva null, ya que supuestamente se arreglo el deadlock. Si llegara a suceder
		//tendria que agregarlo a la cola de bloqueos otra vez, para que tenga una batalla pokemon.
		if(pokemon == NULL){
			log_trace(log_deadlock, "%s se quedo sin pokemon %s, lo bloqueo", batalla->entrenador->msjEntrenador->nombre
					, idPokemon);
			popEntrenador(colaBlock, &mutexColaBlock, batalla->entrenador);
			addToBlock(batalla->entrenador);
			//Si la cola bloqueados tiene solo un entrenador, no hay ningun hilo block. Lo inicio
			if(queue_size(colaBlock) == 1){
				pthread_mutex_lock(&mutexGestionHilos);
				pthread_cond_signal(&condBlockEmpty);
				pthread_mutex_unlock(&mutexGestionHilos);
			}

		}else{
			log_trace(log_deadlock, "Envio un %s para %s", pokemon->nombrePokemon, batalla->entrenador->msjEntrenador->nombre);

			batalla->entrenador->msjEntrenador->accion = CAPTURAR_POKEMON;
			agregarPokemon(batalla->entrenador, pokemon);
			//Envio dos mensajes al entrenador, uno con la accion capturar porque gano la pelea, y otro con el pokemon
			enviarMsjEntrenador(batalla->entrenador->socket,batalla->entrenador->msjEntrenador);
			enviarPokemon(batalla->entrenador->socket, pokemon);

			popEntrenador(colaBlock, &mutexColaBlock, batalla->entrenador);

			//Espero un nuevo mensaje con el pedido de ubicacion pokenest (o de objetivo cumplido)
			batalla->entrenador->msjEntrenador = recibirMsjEntrenador(batalla->entrenador->socket);

			if(batalla->entrenador->msjEntrenador == NULL ||
					batalla->entrenador->msjEntrenador->accion == OBJETIVO_CUMPLIDO)
			{
				if(batalla->entrenador->msjEntrenador->accion == OBJETIVO_CUMPLIDO)
					log_trace(log_deadlock, "%s cumplio su objetivo!!", batalla->entrenador->msjEntrenador->nombre);

				char *nombreMapa = string_from_format("%s", NOMBRE_MAPA);
				mostrarPantalla(nombreMapa);
				free(nombreMapa);

			}else{
				log_trace(log_deadlock, "Agrego a %s en la cola Ready. ACCION %d", batalla->entrenador->msjEntrenador->nombre,
						batalla->entrenador->msjEntrenador->accion);
				addToReady(batalla->entrenador);
				//Si la cola tiene solo un entrenador, el hilo planificador no esta levantado
				if(queue_size(colaReady) == 1){
					pthread_mutex_lock(&mutexGestionHilos);
					pthread_cond_signal(&condPlanificador);
					pthread_mutex_unlock(&mutexGestionHilos);
				}
			}

		}
		free(batalla);
		free(idPokemon);
	}

	list_iterate(peleadores, iterator);
	list_destroy(peleadores);
}

void freeDataDictionary(void *data){

	t_dictionary *dic = (t_dictionary *) data;
	dictionary_destroy_and_destroy_elements(dic, free);
}

/*int main(int argc, char **argv) {

	t_dictionary *dic = dictionary_create();
	t_cantidad *cant = malloc(sizeof(t_cantidad));
	cant->cantidad = 2;
	dictionary_put(dic,"asd", cant);

	void iterator(char*key, void *value){
		t_cantidad *cant = (t_cantidad *) value;
		cant->cantidad = cant->cantidad + 1;
	}

	dictionary_iterator(dic, iterator);

	void iteratorDos(char*key, void *value){
			t_cantidad *cant = (t_cantidad *) value;
			printf("%d", cant->cantidad);
		}

	dictionary_iterator(dic, iteratorDos);



}*/
