/*
 * mapa.c
 *
 *  Created on: 3/9/2016
 *      Author: FacundoCastellano
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/string.h>
#include <sys/types.h>
#include <dirent.h>
#include <pthread.h>
#include "mapa.h"
#include "interfaz_grafica.h"

int string_to_int(char *string);

void cargarMapa(){

	char *pathMapa = string_from_format("%sMapas/%s", PATH_POKEDEX, NOMBRE_MAPA);
	char *pathMetadata = string_from_format("%s/metadata", pathMapa);
	char *pathPokenest = string_from_format("%s/PokeNests", pathMapa);

	t_config* config = config_create(pathMetadata);

	mapa = malloc(sizeof(t_mapa));

	mapa->tiempoChequeoDeadlock = config_get_int_value(config, "TiempoChequeoDeadlock");
	mapa->batalla = config_get_int_value(config, "Batalla");
	char *algoritmo = config_get_string_value(config, "algoritmo");
	if(string_equals_ignore_case(algoritmo, "RR"))
		mapa->algoritmo = ALGORITMO_RR;
	else
		mapa->algoritmo = ALGORITMO_SRDF;
	mapa->quantum = config_get_int_value(config, "quantum");
	mapa->retardo = config_get_int_value(config, "retardo");
	mapa->ip = config_get_string_value(config, "IP");
	mapa->puerto = config_get_int_value(config, "Puerto");

	log_trace(log_mapa, "El mapa cargo su configuracion correctamente. Puerto: %d", mapa->puerto);

	pokenests = dictionary_create();
	recorrerPokenests(pathPokenest);
	free(pathMapa);
	free(pathMetadata);
	free(pathPokenest);
	config_destroy(config);
}

void recorrerPokenests(char *path){

	//Me encuentro en la carpeta pokenest. De cada directorio obtengo su path y cargo la pokenest.
	DIR *dir;
	struct dirent *ent;
	dir = opendir (path);

	if (dir != NULL)
	{
		while ((ent = readdir(dir)) != NULL){

			if ((strcmp(ent->d_name, ".")!=0) && (strcmp(ent->d_name, "..")!=0)){
				//Mientras haya algo en el directorio verifico si es una carpeta
				if(ent->d_type == DT_DIR){
					//Si es una carpeta, es el directorio de una pokenest
					char *nombrePokemon = string_from_format("%s", ent->d_name);
					char *pathPokemon = string_from_format("%s/%s", path, nombrePokemon);
					//Cargo la pokenest
					cargarPokenest(pathPokemon, nombrePokemon);
					free(nombrePokemon);
					free(pathPokemon);
				}
		   }
		}

		(void) closedir (dir);
	}

}

void cargarPokenest(char* path, char *nombrePokemon){

	t_pokenest *pokenest = malloc(sizeof(t_pokenest));

	char *configPath = string_from_format("%s/metadata", path);
	t_config* config = config_create(configPath);
	free(configPath);

	pokenest->nombrePokemon = string_duplicate(nombrePokemon);
	pokenest->tipo = config_get_string_value(config, "Tipo");
	pokenest->identificador = config_get_string_value(config, "Identificador");
	char *posicion = config_get_string_value(config, "Posicion");
	char **posiciones = string_split(posicion, ";");
	pokenest->posicionX = string_to_int(posiciones[0]);
	pokenest->posicionY = string_to_int(posiciones[1]);
	pokenest->pokemons = queue_create();
	obtenerPokemons(path, pokenest->pokemons, nombrePokemon);
	pokenest->cantidadPokemons = queue_size(pokenest->pokemons);

	free(posicion);
	free(posiciones);

	//Lo agrego a la lista de pokenests
	pthread_mutex_lock(&mutexPokenests);
		dictionary_put(pokenests, pokenest->identificador, pokenest);
	pthread_mutex_unlock(&mutexPokenests);
}

void obtenerPokemons(char *path, t_queue *pokemons, char *nombrePokemon){

	struct dirent *ent = NULL;
	DIR *dir = opendir (path);

	if (dir != NULL)
	{
		while ((ent = readdir(dir)) != NULL){

			if ((strcmp(ent->d_name, ".")!=0) && (strcmp(ent->d_name, "..")!=0)){
				//Mientras haya algo en el directorio verifico si es un archivo
				if(ent->d_type == DT_REG){

					if(!string_equals_ignore_case("metadata", ent->d_name)){

						char *pathPokemon = string_from_format("%s/%s", path, ent->d_name);
						char *nombreArchivo = string_from_format("%s", ent->d_name);
						char *nombre = string_duplicate(nombrePokemon);

						t_info_pokemon *pokemon = malloc(sizeof(t_info_pokemon));
						t_config* config = config_create(pathPokemon);
						pokemon->nivel = config_get_int_value(config, "Nivel");
						pokemon->nombreArchivo = nombreArchivo;
						pokemon->nombrePokemon = nombre;
						queue_push(pokemons, pokemon);
						free(pathPokemon);
						config_destroy(config);
					}
				}
		   }
		}

		(void) closedir (dir);
		free(ent);
	}
}

void cargarGui(){

	guiMapa = list_create();
	pthread_mutex_init (&mutexGuiMapa, NULL);
	//Agrego las pokenests al mapa
	agregarPokenests();

	nivel_gui_inicializar();
	int x = 80;
	int y = 25;
	nivel_gui_get_area_nivel(&x,&y);
	char *nombreMapa = string_from_format("%s", NOMBRE_MAPA);
	mostrarPantalla(nombreMapa);
	free(nombreMapa);
}

void cleanMapa(){
	//Termino GUI
	pthread_mutex_lock(&mutexGuiMapa);
	nivel_gui_terminar();
	list_clean_and_destroy_elements(guiMapa, free);
	pthread_mutex_unlock(&mutexGuiMapa);
	log_trace(log_mapa, "Memoria utilizada por GUI liberada");

	//Limpio pokenests
	limpiarPokenests();
	log_trace(log_mapa, "Memoria utilizada por las pokenests liberadas");

	//Elimino colas
	matarListasColas();
	log_trace(log_mapa, "Colas eliminadas");

	log_destroy(log_mapa);
}

void limpiarPokenests(){

	pthread_mutex_lock(&mutexPokenests);

		void pokenestDestroy(void *value){

			t_pokenest *pokenest = (t_pokenest *) value;
			free(pokenest->identificador);
			free(pokenest->nombrePokemon);
			free(pokenest->tipo);
			//Libero la cola de pokemones
			while(!queue_is_empty(pokenest->pokemons)){

				t_info_pokemon *pokemon = queue_pop(pokenest->pokemons);
				free(pokemon->nombreArchivo);
				free(pokemon->nombrePokemon);
				free(pokemon);
			}
			queue_destroy(pokenest->pokemons);
			//Libero la pokenest
			free(pokenest);

		}
		dictionary_clean_and_destroy_elements(pokenests, pokenestDestroy);

	pthread_mutex_unlock(&mutexPokenests);
}

int string_to_int(char *string)
{
	string_append(&string, "\0");

	int entero = strtol(string, NULL, 10);
	free(string);

	return entero;
}
