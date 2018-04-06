/*
 * pokemon_utils.c
 *
 *  Created on: 5/10/2016
 *      Author: Facundo Castellano
 */
#include "pokemon_utils.h"

char *obtenerDirectorioPokemon(char **nombrePokemon);

t_info_pokemon *obtenerPokemon(char *idPokemon){

	t_info_pokemon *pokemon = NULL;

	pthread_mutex_lock(&mutexPokenests);
	t_pokenest *pokenest = dictionary_get(pokenests, idPokemon);

	if(pokenest->cantidadPokemons > 0){
		//Le saco un pokemon en la gui
		sacarDePokenest(pokenest->identificador);
		pokenest->cantidadPokemons = pokenest->cantidadPokemons - 1;

		pokemon = queue_pop(pokenest->pokemons);
	}
	pthread_mutex_unlock(&mutexPokenests);

	return pokemon;
}

void agregarPokemon(t_info_entrenador *entrenador, t_info_pokemon *pokemon){

	t_dictionary *pokemonsAtrapados = entrenador->pokemonsAtrapados;
	t_recursos *cantidadPokemon = NULL;

	//Si el entrenador ya tiene atrapado un mismo pokemon, lo agrego en la cola.
	char *idPokemon = string_from_format("%c", entrenador->msjEntrenador->pokemon);
	if(dictionary_has_key(pokemonsAtrapados, idPokemon)){
		cantidadPokemon = dictionary_get(pokemonsAtrapados, idPokemon);
		queue_push(cantidadPokemon->pokemons, pokemon);
		cantidadPokemon->cantidad = cantidadPokemon->cantidad + 1;
		free(idPokemon);
	}else{
		cantidadPokemon = malloc(sizeof(t_recursos));
		t_queue *colaPokemon = queue_create();
		queue_push(colaPokemon, pokemon);
		cantidadPokemon->pokemons = colaPokemon;
		cantidadPokemon->cantidad = 1;
		dictionary_put(pokemonsAtrapados, idPokemon, cantidadPokemon);
		free(idPokemon);
	}
}

/*
 * Esta funcion retorna el directorio pokenest a partir del mapa y el caracter del pokemon a buscar
 */
char *obtenerDirectorioPokemon(char **nombrePokemon){

	DIR *dirPokenest;
	DIR *dirPokemon;
	struct dirent *entPokenest;
	struct dirent *entPokemon;
	char *pathPokenest = string_from_format("%sMapas/%s/PokeNests", PATH_POKEDEX, NOMBRE_MAPA);
	dirPokenest = opendir (pathPokenest);
	char *pathPokemon;
	bool finalizado;

	if (dirPokenest != NULL)
	{
		while ((entPokenest = readdir(dirPokenest)) != NULL && !finalizado){

			if ((strcmp(entPokenest->d_name, ".")!=0) && (strcmp(entPokenest->d_name, "..")!=0)){
				//Mientras haya algo en el directorio verifico si es una carpeta
				if(entPokenest->d_type == DT_DIR){
					//Asumo que si la carpeta empieza con la letra del pokemon, es el de la pokenest (SOLO HABRIA UNA POKENEST POR LETRA)
					if(string_starts_with(entPokenest->d_name, *nombrePokemon)){
						//Agrego el nombre del pokemon
						*nombrePokemon = string_from_format("%s", entPokenest->d_name);
						pathPokemon = string_from_format("%s/%s", pathPokenest, entPokenest->d_name);
						//Ingreso y obtengo el path del .dat de un pokemon
						dirPokemon = opendir(pathPokemon);
						if (dirPokemon != NULL){
							while ((entPokemon = readdir(dirPokemon)) != NULL && !finalizado){
								if(string_ends_with(entPokemon->d_name, ".dat")){
									string_append_with_format(&pathPokemon, "/%s", entPokemon->d_name);
									finalizado = true;
								}
							}
							(void) closedir (dirPokemon);
						}

					}
				}
		   }
		}

		(void) closedir (dirPokenest);
	}
	free(pathPokenest);
	return pathPokemon;
}
