/*

 * mapa.c
 *
 *  Created on: 3/9/2016
 *      Author: FacundoCastellano
 */

#include <stdlib.h>
#include <stdio.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include <commons/collections/dictionary.h>
#include <commons/collections/queue.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "mapa.h"
#include "planificador.h"
#include "socketes_servidor.h"

int getMaxDescriptor(int *tabla, int n);
void compactaClaves (int *tabla, int *n);
bool keepRunning = true;

int main(int argc, char *argv[]) {

	void sig_handler(int signo)
	{
		if (signo == SIGUSR2){
			recargarPlanificador(mapa);
			log_trace(log_mapa, "Se recargo la configuracion del mapa");
		}else if(signo == SIGINT){
			cleanMapa();
			closeSocket(servidor);
			free(servidor);
			keepRunning = false;
			pthread_kill(idHiloDeadlock, 0);
			pthread_kill(idHiloBlock, 0);
			pthread_kill(idHiloPlanificador, 0);
		}
	}

	if(argc > 1){
		char *nombre = string_new();
		char *path = "";
		int i = 0;
		//Espero el nombre del mapa (puede tener espacios) y el path (/pokedex/)
		for(i=1;i<argc;i++){

			//Si empieza con . o / es un path
			if(string_starts_with(argv[i], "/")){
				path = string_from_format("%s", argv[i]);
				//Si el path no tiene un / al final, se lo pongo
				if(!string_ends_with(path, "/"))
					string_append(&path, "/");
				//Copio el path a la variable global
				strncpy(PATH_POKEDEX, path, string_length(path));

				free(path);

			}else{
				//Es el nombre del mapa
				string_append(&nombre, argv[i]);
				char *space = string_from_format(" ");
				string_append(&nombre, space);
				free(space);
			}
		}

		//Si el nombre del mapa quedo con espacios de mas, se lo saco
		if(string_ends_with(nombre, " ")){
			char *subname = string_substring_until(nombre, string_length(nombre) - 1);
			free(nombre);
			nombre = subname;
		}

		strncpy(NOMBRE_MAPA, nombre, string_length(nombre));
		free(nombre);
	}

	signal(SIGUSR2, sig_handler);
	signal(SIGINT, sig_handler);
	char *nombreMapa = string_from_format("%s", NOMBRE_MAPA);
	log_mapa = log_create("mapa.log",NOMBRE_MAPA,FALSE,LOG_LEVEL_TRACE);
	fd_set descriptores;
	int descriptoresClientes[100];
	t_msj_entrenador* msjEntrenador = NULL;

	int maxDescriptor = 0;
	int resultSelect = 0;

	crearListasColas();

	//Cargo las configuraciones del mapa
	cargarMapa();

	//Inicio el guiMapa;
	cargarGui();

	pthread_mutex_init(&mutexGestionReady, NULL);
	pthread_mutex_init(&mutexGestionBlock, NULL);
	pthread_mutex_init(&mutexGestionHilos, NULL);
	pthread_mutex_init(&mutexBlockDeadlock, NULL);
	pthread_cond_init(&condPlanificador, NULL);
	pthread_cond_init(&condBlock, NULL);
	pthread_cond_init(&condBlockEmpty, NULL);

	log_trace(log_mapa, "Inicio el hilo planificador.");
	pthread_create (&idHiloPlanificador, NULL, (void*) hiloPlanificador, NULL);

	//Creo el servidor
	log_trace(log_mapa, "Inicio el servidor.");
	servidor = socketCreateServer(mapa->puerto);

	if(servidor == NULL){
		log_error(log_mapa, "Hubo un error al iniciar el servidor. Puerto: %d", mapa->puerto);
		log_destroy(log_mapa);
		return EXIT_FAILURE;
	}
	t_list *clientesConectados = list_create();
	int size = list_size(clientesConectados);

	while(keepRunning)
	{
		int i = 0;

		//Inicializo el descriptor
		FD_ZERO(&descriptores);
		//Agrego el servidor al select()
		FD_SET(servidor->descriptor, &descriptores);

		for (i=0; i<size; i++)
			FD_SET (descriptoresClientes[i], &descriptores);

		if(size == 0)
			maxDescriptor = servidor->descriptor;

		//Realizo el select
		resultSelect = select(maxDescriptor+1, &descriptores, NULL, NULL, NULL);

		if (resultSelect > 0) {
			//Veo si hubo un cambio en el descriptor del servidor. Si es asi, es porque hay una nueva conexion disponible
			if(FD_ISSET(servidor->descriptor, &descriptores)){

				t_socket* cliente = socketAcceptClient(servidor);
				if(cliente == NULL){
					log_error(log_mapa, "No se pudo aceptar la conexion del cliente. Descriptor del servidor: %d. Puerto: %d", servidor->descriptor, mapa->puerto);
				}else{

					t_info_entrenador *entrenador = malloc(sizeof(t_info_entrenador));

					//Recibo el mensaje del entrenador con su estructura (TENDRIA QUE TENER UN MSJ DE UBICACION POKENEST)
					msjEntrenador = recibirMsjEntrenador(cliente);

					entrenador->msjEntrenador = msjEntrenador;
					entrenador->socket = cliente;
					entrenador->pokemonsAtrapados = dictionary_create();
					entrenador->quantum = 0;
					entrenador->simbolo = entrenador->msjEntrenador->simbolo;
					entrenador->tiempoDeIngreso = time(NULL);

					log_trace(log_mapa, "El entrenador %s se ha conectado. Simbolo = %c", entrenador->msjEntrenador->nombre, entrenador->simbolo);

					//Lo agrego al GUI
					agregarEntrenador(entrenador->simbolo);

					log_trace(log_mapa, "Se agrega al entrenador %s a la cola de Readys", msjEntrenador->nombre);

					//Lo agrego a la lista de clientes conectados
					FD_SET(cliente->descriptor, &descriptores);
					list_add(clientesConectados, entrenador);
					descriptoresClientes[size++] = entrenador->socket->descriptor;
					//Busco un nuevo maxDescriptor
					maxDescriptor = getMaxDescriptor(descriptoresClientes, size);
					//Agrego la estructura a la cola de listos
					addToReady(entrenador);
					//Si la cola tiene solo un entrenador, el planificador esta bloqueado
					if(queue_size(colaReady) == 1){
						pthread_mutex_lock(&mutexGestionReady);
						pthread_cond_signal(&condPlanificador);
						pthread_mutex_unlock(&mutexGestionReady);
					}

				}

			}
			//Verifico si algun cliente se desconecto

			for(i=0;i<size;i++){

				if(FD_ISSET(descriptoresClientes[i], &descriptores)){
					//Si devuelve 0, es porque se desconecto.
					char buf[10];
					if(recv(descriptoresClientes[i], buf, 10, MSG_PEEK)<1){

						t_info_entrenador *entrenador;
						int j = 0;
						for(j=0;j<size;j++){
							entrenador = list_get(clientesConectados, j);
							if(descriptoresClientes[i] == entrenador->socket->descriptor)
								break;
						}

						log_trace(log_mapa, "El entrenador %c se desconecto :/", entrenador->simbolo);
						borrarEntrenador(entrenador->simbolo);
						//if(entrenador->pokemonsAtrapados != NULL)
						//	liberarRecursos(entrenador);
						popEntrenador(colaBlock, &mutexColaBlock, entrenador);
						mostrarPantalla(nombreMapa);

						if(!queue_is_empty(colaBlock)){
							pthread_mutex_lock(&mutexGestionBlock);
							pthread_cond_signal(&condBlock);
							pthread_mutex_unlock(&mutexGestionBlock);
						}

						list_remove(clientesConectados, j);

						//Borro el descriptor
						descriptoresClientes[i] = -1;
						compactaClaves (descriptoresClientes, &size);

						//Busco un maxDescriptor
						if(!list_is_empty(clientesConectados))
							maxDescriptor = getMaxDescriptor(descriptoresClientes, size);
					}
				}
			}

			struct timespec tim;
			tim.tv_sec = 0;
			tim.tv_nsec = 300 * 1000000;
			nanosleep(&tim, NULL);
		}
	}

	return EXIT_SUCCESS;
}

int getMaxDescriptor(int *tabla, int n){

	int i;
	int max;

	max = tabla[0];
	for (i=0; i<n; i++)
		if (tabla[i] > max)
			max = tabla[i];

	return max;
}

void compactaClaves (int *tabla, int *n)
{
	int i,j;

	if ((tabla == NULL) || ((*n) == 0))
		return;

	j=0;
	for (i=0; i<(*n); i++)
	{
		if (tabla[i] != -1)
		{
			tabla[j] = tabla[i];
			j++;
		}
	}

	*n = j;
}

