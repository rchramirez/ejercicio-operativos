#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/string.h>
#include <signal.h>
#include "entrenador.h"

void sig_handler(int signo)
{
	if (signo == SIGUSR1){
		//El entrenador recibe una nueva vida
		entrenador->vidas = entrenador->vidas + 1;
		log_info(log_entrenador, "Se me sumo una vida. VIDAS RESTANTES = %d", entrenador->vidas);

	}else if(signo == SIGTERM){
		//El entrenador pierde una vida
		if(entrenador->vidas > 1){
			entrenador->vidas = entrenador->vidas - 1;
			log_info(log_entrenador, "Se me quito una vida. VIDAS RESTANTES = %d", entrenador->vidas);
		}else{
			log_info(log_entrenador, "Se quedo sin vidas", entrenador->vidas);
			closeSocket(cliente->ptrSocket);
			printf("Perdiste. Lo reintentaste %d veces. Desea reiniciar el juego? s/n \n", entrenador->reintentos);

			char respuesta=getc(stdin);
			borrarPokemons();

			if(respuesta == 's'){

				int reintentosActual = entrenador->reintentos + 1;
				log_info(log_entrenador, "Decidi jugar nuevamente. Reintentos: %d", reintentosActual);
				recargarEntrenador(entrenador);
				entrenador->reintentos = reintentosActual;
				initEntrenador();
				exit(EXIT_SUCCESS);

			}else{
				log_info(log_entrenador, "Me rendi \n");
				freeEntrenador(entrenador);
				exit(EXIT_SUCCESS);
			}
		}
	}
}

int main(int argc, char *argv[]){

	if(argc > 1){

		char *path;
		//Espero el nombre del entrenador y el path (/pokedex/)
		strncpy(ENTRENADOR_NOMBRE, argv[1], string_length(argv[1]));
		//Path
		path = string_from_format("%s", argv[2]);
		//Si el path no tiene un / al final, se lo pongo
		if(!string_ends_with(path, "/")){
			string_append(&path, "/");
		}
		//Copio el path a la variable global
		strncpy(POKEDEX_PATH, path, string_length(path));
	}

	//signal(SIGINT, rutina); INTERRUMPLE EL SELECT Y RECV, HAY QUE LEVANTARLOS MANUALMENTE
	signal(SIGUSR1, sig_handler);
	signal(SIGTERM, sig_handler);

	cargarEntrenador();
	log_entrenador = log_create("entrenador.log", entrenador->nombre, FALSE, LOG_LEVEL_INFO);

	//Empieza la aventura, calculo su tiempo de inicio
	monitoreo = malloc(sizeof(t_monitoreo));
	monitoreo->tiempoTotal = time(NULL);
	monitoreo->tiempoBloqueado = 0;
	monitoreo->cantidadDeadlocks = 0;
	monitoreo->cantidadMuertes = 0;

	initEntrenador();

	return EXIT_SUCCESS;
}

int iniciarMapa(t_info_mapa *mapa, t_msj_entrenador *msjEntrenador){

	t_info_conexion *infoMapa = malloc(sizeof(t_info_conexion));
	//Armo el path del mapa
	char *pathMapa = string_from_format("%sMapas/%s/metadata", POKEDEX_PATH, mapa->nombre);

	cargarInfoMapa(infoMapa, pathMapa);
	free(pathMapa);

	//Me conecto al mapa
	cliente = socketCreateClientAndConnect(infoMapa->ip, infoMapa->port);

	if(cliente == NULL){
		log_error(log_entrenador, "Hubo un error al conectarme al mapa %s. IP = %s PUERTO = %d", mapa->nombre, infoMapa->ip, infoMapa->port);
		return EXIT_FAILURE;
	}

	log_info(log_entrenador, "Me conecte al mapa %s. IP = %s PUERTO = %d", mapa->nombre, infoMapa->ip, infoMapa->port);

	//Ya empiezo pidiendo la pokenest del primer pokemon
	msjEntrenador->accion = UBICACION_POKENEST;
	char *pokemon = queue_pop(mapa->pokemones);
	msjEntrenador->pokemon = *pokemon;
	msjEntrenador->posicionX = 0;
	msjEntrenador->posicionY = 0;
	entrenador->posX = 0;
	entrenador->posY = 0;
	free(pokemon);
	log_info(log_entrenador, "Busco la posicion de mi primer pokemon. ID POKEMON = %c", msjEntrenador->pokemon);

	//Una vez conectado, le envio al servidor los datos del proceso entrenador
	//Le manda simbolo, cantidad de vidas, reintentos
	enviarMsjEntrenador(cliente->ptrSocket, msjEntrenador);
	free(msjEntrenador);

	free(infoMapa->ip);
	free(infoMapa);

	return gestionarEntrenador(cliente->ptrSocket, mapa);

}

void initEntrenador(){

	log_info(log_entrenador, "Comienzo mi viaje pokemon.");

	t_msj_entrenador *msjEntrenador = malloc(sizeof(t_msj_entrenador));
	//Inicio el mensaje con su protocolo
	msjEntrenador->simbolo = *entrenador->simbolo;
	msjEntrenador->nombre = entrenador->nombre;
	msjEntrenador->vidas = entrenador->vidas;
	msjEntrenador->reintentos = entrenador->reintentos;

	t_list *mapas = entrenador->hojaDeViaje;
	int size = list_size(mapas);
	int var;

	for (var = 0; var < size; ++var) {

		t_info_mapa *mapa = list_get(mapas, var);

		int res = iniciarMapa(mapa, msjEntrenador);

		if(res == OBJETIVO_PERDIDO){

			if(entrenador->vidas > 0){

				log_info(log_entrenador, "Todavia me quedan vidas, me conecto nuevamente al mapa %s. VIDAS RESTANTES = %d",
						mapa->nombre, entrenador->vidas);

				//Recargo los pokemones nuevamente y vuelvo a conectarme al mapa.
				cargarPokemons(mapa);
				entrenador->posX = 0;
				entrenador->posY = 0;
				entrenador->reintentos = entrenador->reintentos + 1;

				--var;

			}else{

				printf("Perdiste. Lo reintentaste %d veces. Desea reiniciar el juego? s/n \n", entrenador->reintentos);

				char respuesta=getc(stdin);
				borrarPokemons();

				if(respuesta == 's'){

					int reintentosActual = entrenador->reintentos + 1;
					log_info(log_entrenador, "Decidi jugar nuevamente. Reintentos: %d", reintentosActual);
					recargarEntrenador(entrenador);
					entrenador->reintentos = reintentosActual;
					initEntrenador();

				}else{
					printf("nos vimo! \n");
					log_info(log_entrenador, "Me rendi");
					freeEntrenador(entrenador);
					return;
				}
			}
		}

	}

	log_info(log_entrenador, "Me converti en maestro pokemon");
	time_t finalAventura = time(NULL);
	monitoreo->tiempoTotal = difftime(finalAventura, monitoreo->tiempoTotal);

	printf("Felicitaciones %s! Te convertiste en maestro pokemon. Tardaste %d segundos en lograrlo, %d atrapando Pokemons. Peleaste %d veces, mueriendo %d veces. \n",
			entrenador->nombre, (int) monitoreo->tiempoTotal, (int) monitoreo->tiempoBloqueado, monitoreo->cantidadDeadlocks, monitoreo->cantidadMuertes);
	exit(EXIT_SUCCESS);
	//freeEntrenador(entrenador);
}

