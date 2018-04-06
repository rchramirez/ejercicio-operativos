#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <commons/log.h>
#include <commons/config.h>
#include <pthread.h>
#include "pokedex_servidor.h"

int main(int argc, char *argv[]) {
	fd_set descriptores;
	int puerto;
	int maxDescriptor;
	int resultSelect;

	log_serv = log_create("servidor.log","serv",FALSE,LOG_LEVEL_TRACE);

	if(argc > 1){
		puerto = atoi(argv[1]);
		//Tomo el path del disco
		int fd = open(argv[2], O_RDWR);
		mapearDisco(fd);
		log_info(log_serv, "El disco %s se ha mapeado", (char*) argv[2]);
	}
	else{
		printf("%s", "Falta indicar el puerto y el path del disco");
		exit(1);
	}

	//pthread_mutex_init(&mutexWrite, NULL);
	pthread_mutex_init(&mutexTabla, NULL);
	pthread_mutex_init(&mutexBitmap, NULL);

	t_socket* servidor = socketCreateServer(puerto);

	if(servidor == NULL){
		log_error(log_serv, "El servidor no pudo crearse");
		return EXIT_FAILURE;
	}
	while(1)
	{
		//Inicializo el descriptor
		FD_ZERO(&descriptores);

		//Agrego el servidor al select()
		FD_SET(servidor->descriptor, &descriptores);

		maxDescriptor = servidor->descriptor;

		//Realizo el select
		resultSelect = select(maxDescriptor+1, &descriptores, NULL, NULL, NULL);

		if (resultSelect > 0) {
			//Veo si hubo un cambio en el descriptor del servidor. Si es asi, es porque hay una nueva conexion disponible
			if(FD_ISSET(servidor->descriptor, &descriptores)){

				t_socket* cliente = socketAcceptClient(servidor);
				if(cliente == NULL){
					log_error(log_serv, "Un cliente se intento conectar pero hubo un error");
				}else{

					//Creo su hilo
					pthread_create (&idHiloConexiones, NULL, (void*) hiloConexiones, cliente);

					//Loggear descriptor del cliente y servidor
					log_info(log_serv, "Se ha conectado un nuevo cliente. DESCRIPTOR = %d", cliente->descriptor);
				}

			}

		}
	}
	return EXIT_SUCCESS;
}
