#include <stdio.h>
#include <stdbool.h>
#include <sys/types.h>
#include <dirent.h>
#include <commons/collections/list.h>
#include <commons/string.h>
#include <commons/log.h>
#include "entrenador.h"
#include <commons/collections/queue.h>

void borrarArchivosEnDirectorio(char *path);

void cargarInfoMapa(t_info_conexion* conexion, char* pathMapa) {
	//Luego de obtener el medata obtengo el IP y Puerto del mapa
	t_config* config = config_create(pathMapa);
	conexion->ip = config_get_string_value(config, "IP");
	conexion->port = config_get_int_value(config, "Puerto");
}

t_list* cargarMapas(){

	char** hojaDeViaje = config_get_array_value(configEntrenador, "hojaDeViaje");
	t_list* listaHojaDeViaje = list_create();

	//Por cada mapa que haya en la hoja de viaje, se creara y agregara a la lista un t_mapa con su respectivo nombre y pokemones a atrapar.
	int i = 0;
	while (hojaDeViaje[i] != NULL) {

		t_info_mapa *mapa = malloc(sizeof(t_info_mapa));
		mapa->nombre = hojaDeViaje[i];

		cargarPokemons(mapa);

		list_add(listaHojaDeViaje, mapa);
		i++;
	}
	free(hojaDeViaje);

	return listaHojaDeViaje;
}

void cargarPokemons(t_info_mapa *mapa){

	//Creo el nombre de la key de los objetivos a atrapar ("obj[PuebloPaleta]")
	char* key = string_from_format("obj[%s]", mapa->nombre);

	char **pokemones = config_get_array_value(configEntrenador, key);
	t_queue *colaPokemon = crearColaPokemon(pokemones);
	free(pokemones);
	free(key);

	mapa->pokemones = colaPokemon;

}

t_queue *crearColaPokemon(char **pokemones){

	int i = 0;
	t_queue *colaPokemon = queue_create();

	while (pokemones[i] != NULL) {
		queue_push(colaPokemon, pokemones[i]);
		i++;
	}

	return colaPokemon;
}

void cargarEntrenador(){

	char *pathConfig = string_from_format("%sEntrenadores/%s/metadata", POKEDEX_PATH, ENTRENADOR_NOMBRE);

	configEntrenador = config_create(pathConfig);
	entrenador = malloc(sizeof(t_entrenador));
	entrenador->nombre = config_get_string_value(configEntrenador, "nombre");
	entrenador->simbolo = config_get_string_value(configEntrenador, "simbolo");
	entrenador->vidas = config_get_int_value(configEntrenador, "vidas");
	entrenador->reintentos = config_get_int_value(configEntrenador, "reintentos");
	entrenador->hojaDeViaje = cargarMapas();
	entrenador->posX = 0;
	entrenador->posY = 0;
	entrenador->pokemones = list_create();

	free(pathConfig);
}

int gestionarEntrenador(t_socket *cliente, t_info_mapa *mapa){

	t_queue *colaMovimientos;
	t_msj_entrenador *mensaje;
	time_t comienzo, final;
	bool mandarMsj = false;

	while(1){

		mensaje = recibirMsjEntrenador(cliente);

		mandarMsj = true;
		int accion = mensaje->accion;

		switch (accion) {
			case UBICACION_POKENEST: {
				//Obtengo la posicion y a partir de ella y mi posicion, calculo la cola de movimientos
				log_info(log_entrenador, "Me llego la ubicacion de la pokenest: X=%d , Y=%d", mensaje->posicionX, mensaje->posicionY);

				colaMovimientos = calcularColaMovimientos(entrenador, mensaje);
				//Si esta vacia es porque estoy en la misma posicion de la pokenest, asi que pido capturar
				if(queue_is_empty(colaMovimientos)){

					log_info(log_entrenador, "Llegue a la posicion de la pokenest %c!. Posicion: X=%d , Y=%d. Pido capturarlo",
							mensaje->pokemon, mensaje->posicionX, mensaje->posicionY);
					queue_destroy(colaMovimientos);
					mensaje->accion = CAPTURAR_POKEMON;
					comienzo = time(NULL);
				}else{
					//Agarro el primer movimiento y me muevo
					t_movimiento *mov = queue_pop(colaMovimientos);
					mensaje->accion = MOVERSE;
					mensaje->movimiento = mov->movimiento;
					free(mov);
				}
				break;
			}
			case MOVERSE: {
				//Si la cola de movimientos esta vacia, estoy en la misma posicion de la pokenest.
				if(queue_is_empty(colaMovimientos)){

					log_info(log_entrenador, "Llegue a la posicion de la pokenest %c!. Posicion: X=%d , Y=%d. Pido capturarlo",
												mensaje->pokemon, mensaje->posicionX, mensaje->posicionY);
					queue_destroy(colaMovimientos);
					//Actualizo mi posicion con la de la pokenest
					entrenador->posX = mensaje->posicionX;
					entrenador->posY = mensaje->posicionY;
					//Pido capturar el pokemon
					mensaje->accion = CAPTURAR_POKEMON;
					comienzo = time(NULL);
				}else{
					//Envio mi proximo movimiento
					t_movimiento *mov = queue_pop(colaMovimientos);
					mensaje->accion = MOVERSE;
					mensaje->movimiento = mov->movimiento;
					free(mov);
				}
				break;
			}
			case CAPTURAR_POKEMON: {

				final = time(NULL);
				monitoreo->tiempoBloqueado += difftime(final, comienzo);

				//SI LLEGA ACA ES PORQUE PUEDE ATRAPAR EL POKEMON QUE PIDIO.
				t_info_pokemon *pokemon = recibirPokemon(cliente);
				log_info(log_entrenador, "Recibi el pokemon %s", pokemon->nombrePokemon, entrenador->nombre);

				//Lo guardo en mi lista de atrapados.
				list_add(entrenador->pokemones, pokemon);
				guardarEnLoDeBill(mapa->nombre, mensaje->pokemon, pokemon);

				//Me fijo mi proximo objetivo. Si no tengo nada en la cola es porque ya capture todos
				if(queue_is_empty(mapa->pokemones)){

					obtenerMedalla(mapa->nombre);
					log_info(log_entrenador, "Termine el mapa %s y recibi la medalla", mapa->nombre);
					borrarPokemons();
					log_info(log_entrenador, "Le aviso al mapa %s que termine", mapa->nombre);
					//Le aviso al mapa que termine.
					mensaje->accion = OBJETIVO_CUMPLIDO;
					enviarMsjEntrenador(cliente, mensaje);

					closeSocket(cliente);
					free(cliente);

					return OBJETIVO_CUMPLIDO;
				}else{
					//Pido la ubicacion de la pokenest de mi proximo pokemon
					char *proximoPokemon = queue_pop(mapa->pokemones);
					mensaje->accion = UBICACION_POKENEST;
					mensaje->pokemon = *proximoPokemon;
					free(proximoPokemon);
					log_info(log_entrenador, "Sigo atrapando pokemones en %s. Pido la posicion de la pokenest %c. %d", mapa->nombre, mensaje->pokemon, mensaje->accion);
				}
				break;
			}
			case BATALLA_POKEMON: {
				//MONITOREO
				monitoreo->cantidadDeadlocks++;

				//Elijo a mi mejor pokemon
				t_info_pokemon *pokemon = getMejorPokemon(entrenador->pokemones);
				log_info(log_entrenador, "Batalla pokemon! Mando a %s, mi mejor pokemon. Lvl %d", pokemon->nombrePokemon, pokemon->nivel);
				//Lo envio para la batalla
				enviarPokemon(cliente, pokemon);
				mandarMsj = false;
				break;
			}
			case MUERTE_ENTRENADOR: {

				//MONITOREO
				final = time(NULL);
				monitoreo->tiempoBloqueado += difftime(final, comienzo);
				monitoreo->cantidadMuertes++;

				//Le resto una vida
				entrenador->vidas -= 1;
				log_info(log_entrenador, "Mori x_x . Vidas restantes = %d", entrenador->vidas);
				closeSocket(cliente);
				free(cliente);
				log_info(log_entrenador, "Me desconecte del mapa %s", mapa->nombre);
				return OBJETIVO_PERDIDO;
			}

		}

		if(mandarMsj){
			enviarMsjEntrenador(cliente, mensaje);
			free(mensaje->nombre);
			free(mensaje);
		}
	}

	return EXIT_SUCCESS;

}

t_queue *calcularColaMovimientos(t_entrenador *entrenador, t_msj_entrenador *mensajeEntrenador){

	//Obtengo una cola posY (contendran los movimientos UP y DOWN) y una cola posX (RIGHT, LEFT)
	t_queue *colaPosY = obtenerColaPosY(entrenador->posY, mensajeEntrenador->posicionY);
	t_queue *colaPosX = obtenerColaPosX(entrenador->posX, mensajeEntrenador->posicionX);

	//Intercalo los movimientos
	t_queue *colaMovimiento = intercarlarColas(colaPosY, colaPosX);

	queue_destroy(colaPosY);
	queue_destroy(colaPosX);

	return colaMovimiento;
}

t_queue *obtenerColaPosY(int posEntrenador, int posPokenest){

	t_queue *colaPosY = queue_create();

	int i = 0;
	//Obtengo la diferencia entre la posicion de la pokenest y el entrenador
	int diferencia = posPokenest - posEntrenador;
	//Si la diferencia es mayor a 0 entonces la pokenest se encuentra arriba. Si es 0 no agrego nada y retorno
	//la cola vacia
	if(diferencia > 0){
		//Agrego los movimientos UP
		for(i=0;i<diferencia;i++){
			t_movimiento *mov = malloc(sizeof(t_movimiento));
			mov->movimiento = MOVE_UP;
			queue_push(colaPosY, mov);
		}
	}else if(diferencia < 0){
		//Agrego los movimientos DOWN
		for(i=0;i>diferencia;i--){
			t_movimiento *mov = malloc(sizeof(t_movimiento));
			mov->movimiento = MOVE_DOWN;
			queue_push(colaPosY, mov);
		}
	}

	return colaPosY;
}

t_queue *obtenerColaPosX(int posEntrenador, int posPokenest){

	t_queue *colaPosX = queue_create();

	int i = 0;
	//Obtengo la diferencia entre la posicion de la pokenest y el entrenador
	int diferencia = posPokenest - posEntrenador;
	//Si la diferencia es mayor a 0 entonces la pokenest se encuentra a la derecha.
	//Si es 0 no agrego nada y retorno la cola vacia
	if(diferencia > 0){
		//Agrego los movimientos RIGHT
		for(i=0;i<diferencia;i++){
			t_movimiento *mov = malloc(sizeof(t_movimiento));
			mov->movimiento = MOVE_RIGHT;
			queue_push(colaPosX, mov);
		}
	}else if(diferencia < 0){
		//Agrego los movimientos LEFT
		for(i=0;i>diferencia;i--){
			t_movimiento *mov = malloc(sizeof(t_movimiento));
			mov->movimiento = MOVE_LEFT;
			queue_push(colaPosX, mov);
		}
	}

	return colaPosX;
}

t_queue *intercarlarColas(t_queue *colaPosY, t_queue *colaPosX){

	t_queue *colaMovimiento = queue_create();

	//Mientras haya elementos en una de las dos colas, los agrego de forma intercalada
	while(!queue_is_empty(colaPosY) || !queue_is_empty(colaPosX)){

		if(!queue_is_empty(colaPosY))
			queue_push(colaMovimiento, queue_pop(colaPosY));
		if(!queue_is_empty(colaPosX))
			queue_push(colaMovimiento, queue_pop(colaPosX));
	}

	return colaMovimiento;

}

t_info_pokemon *getMejorPokemon(t_list *pokemones){

	bool comparator(void *pokemonA, void *pokemonB){

		t_info_pokemon *unPokemon = (t_info_pokemon *) pokemonA;
		t_info_pokemon *otroPokemon = (t_info_pokemon *) pokemonB;

		return unPokemon->nivel > otroPokemon->nivel;
	}

	list_sort(pokemones, comparator);

	return list_get(pokemones, 0);
}

void guardarEnLoDeBill(char *nombreMapa, char idPokemon, t_info_pokemon *pokemon){

	//Creo los path del entrenador y del mapa.
	char *pathEntrenador = string_from_format("%sEntrenadores/%s/Dir de Bill/", POKEDEX_PATH, ENTRENADOR_NOMBRE);
	char *stringIdPokemon = string_from_format("%c", idPokemon);
	char *pathPokemon = obtenerDirectorioPokemon(nombreMapa, stringIdPokemon);
	free(stringIdPokemon);

	string_append_with_format(&pathPokemon, "/%s", pokemon->nombreArchivo);
	//Hago la copia al path del entrenador
	copiarEnDirectorio(pathPokemon, pathEntrenador);

	log_info(log_entrenador, "Copie el archivo %s a mi directorio de Bill", pokemon->nombreArchivo);
}

/*
 * Esta funcion retorna el directorio pokenest a partir del mapa y el caracter del pokemon a buscar
 */
char *obtenerDirectorioPokemon(char *nombreMapa, char *idPokemon){

	struct dirent *entPokenest;
	DIR *dirPokenest;
	char *pathPokenest = string_from_format("%sMapas/%s/PokeNests", POKEDEX_PATH, nombreMapa);
	dirPokenest = opendir (pathPokenest);
	char *pathPokemon;

	if (dirPokenest != NULL)
	{
		while ((entPokenest = readdir(dirPokenest)) != NULL){

			if ((strcmp(entPokenest->d_name, ".")!=0) && (strcmp(entPokenest->d_name, "..")!=0)){
				//Mientras haya algo en el directorio verifico si es una carpeta
				if(entPokenest->d_type == DT_DIR){
					//Asumo que si la carpeta empieza con la letra del pokemon, es el de la pokenest (SOLO HABRIA UNA POKENEST POR LETRA)
					if(string_starts_with(entPokenest->d_name, idPokemon)){
						pathPokemon = string_from_format("%s/%s", pathPokenest, entPokenest->d_name);
					}
				}
		   }
		}
		rewinddir(dirPokenest);
		closedir (dirPokenest);
	}
	free(pathPokenest);
	return pathPokemon;
}

void obtenerMedalla(char *nombreMapa){

	char *pathEntrenador = string_from_format("%sEntrenadores/%s/medallas/", POKEDEX_PATH, ENTRENADOR_NOMBRE);
	char *pathMedalla = string_from_format("%sMapas/%s/medalla-%s.jpg", POKEDEX_PATH, nombreMapa, nombreMapa);

	copiarEnDirectorio(pathMedalla, pathEntrenador);

}

void borrarPokemons(){
	char *pathEntrenador = string_from_format("%sEntrenadores/%s/Dir de Bill/", POKEDEX_PATH, ENTRENADOR_NOMBRE);
	borrarArchivosEnDirectorio(pathEntrenador);
	//borro los pokemones atrapados
	list_clean(entrenador->pokemones);
}

void borrarArchivosEnDirectorio(char *path){
   path = modificarEspaciosEntreDirectorios(path);
   char *rm = string_from_format("/bin/rm %s*", path);
   char command[string_length(rm)];
   strcpy(command, rm);
   system(rm);
   free(path);
   free(rm);
}

void copiarEnDirectorio(char *pathFrom, char *pathTo){

   pathFrom = modificarEspaciosEntreDirectorios(pathFrom);
   pathTo = modificarEspaciosEntreDirectorios(pathTo);

   char *cp = string_from_format("/bin/cp %s %s", pathFrom, pathTo);

   char command[string_length(cp)];

   strcpy(command, cp);
   system(cp);
   free(pathFrom);
   free(pathTo);
   free(cp);
}

char *modificarEspaciosEntreDirectorios(char *path){

	char **cadenas = string_split(path, " ");
	char *nuevoPath = string_new();
	void iterator(char *value){
		string_append_with_format(&nuevoPath, "%s\\ ", value);
		free(value);
	}
	string_iterate_lines(cadenas, iterator);

	char *subNuevoPath = string_substring_until(nuevoPath, string_length(nuevoPath) -2);
	free(nuevoPath);
	free(path);
	free(cadenas);

	return subNuevoPath;
}

void recargarEntrenador(t_entrenador *entrenador){

	entrenador->vidas = config_get_int_value(configEntrenador, "vidas");
	entrenador->hojaDeViaje = cargarMapas();
	entrenador->posX = 0;
	entrenador->posY = 0;
	entrenador->pokemones = list_create();

}

void freeEntrenador(t_entrenador *entrenador){

	void freeHojaDeViaje(void *mapa){

		t_info_mapa *infoMapa = (t_info_mapa *) mapa;
		free(infoMapa->nombre);

		while(!queue_is_empty(infoMapa->pokemones)){

			t_info_pokemon *pokemon = queue_pop(infoMapa->pokemones);
			free(pokemon->nombreArchivo);
			free(pokemon->nombrePokemon);
			free(pokemon);
		}
		queue_destroy(infoMapa->pokemones);
		free(infoMapa);
	}

	list_destroy_and_destroy_elements(entrenador->hojaDeViaje, freeHojaDeViaje);

	void freePokemones(void *value){

		t_info_pokemon *pokemon = (t_info_pokemon *) value;
		free(pokemon->nombreArchivo);
		free(pokemon->nombrePokemon);
		free(pokemon);
	}

	list_destroy_and_destroy_elements(entrenador->pokemones, freePokemones);

	free(entrenador->simbolo);
	free(entrenador->nombre);
	free(entrenador);
}
