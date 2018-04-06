/*

 * main.c
 *
 *  Created on: 18/9/2016
 *      Author: utnso



#include "pokedex_cliente.h"

 *  Dentro de los argumentos que recibe nuestro programa obligatoriamente
 * debe estar el path al directorio donde vamos a montar nuestro FS
 *
 * ARGS: PATH - IP - PORT


int main(int argc, char *argv[]) {
	pokedexLog = log_create("pokedex_cliente.log", "POKEDEX_CLIENTE", 1,
			LOG_LEVEL_DEBUG);

	char *ip = "127.0.0.1";
	int puerto = 8080;

	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	//Me conecto al pokedex_servidor
	pokedexServer = socketCreateClientAndConnect(ip, puerto); //ip y port se obtienen por variable de entorno

	if (pokedexServer == NULL) {
		log_error(pokedexLog,
				"Hubo un error al conectarme al servidor en IP = %s y PUERTO = %d",
				ip, puerto);
		return EXIT_FAILURE;
	}
	log_info(pokedexLog, "Me conecte al servidor. IP = %s PUERTO = %d", ip,
			puerto);

	// Limpio la estructura que va a contener los parametros
	memset(&runtime_options, 0, sizeof(struct t_runtime_options));

	// Esta funcion de FUSE lee los parametros recibidos y los intepreta
	if (fuse_opt_parse(&args, &runtime_options, fuse_options, NULL) == -1) {
		* error parsing options
		perror("Invalid arguments!");
		return EXIT_FAILURE;
	}

	// Si se paso el parametro --welcome-msg
	// el campo welcome_msg deberia tener el
	// valor pasado
	if (runtime_options.welcome_msg != NULL) {
		printf("%s\n", runtime_options.welcome_msg);
	}

	// Esta es la funcion principal de FUSE, es la que se encarga
	// de realizar el montaje, comunicarse con el kernel, delegar todo
	// en varios threads
	return fuse_main(args.argc, args.argv, &poke_oper, NULL);
}

*/
