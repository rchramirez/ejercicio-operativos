/*
 * socketes_servidor.c
 *
 *  Created on: 6/9/2016
 *      Author: Facundo Borda
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <commons/log.h>
#include "tipos.h"
#include "socketes_servidor.h"

t_socket* socketCreate();

/*
 * @NAME: socketCreateServer
 * @DESC: Crea un socket para ser utilizado como server. Realiza los procesos de Socket y Bind.
 * Devuelve un struct con el estado de creacion y el descriptor correspondiente.
 * @PARAMS:
 *		port	: puerto de escucha
 */
t_socket* socketCreateServer(Int32U port) {

	t_log* log = log_create("sockets.log","SOCKET_SERVIDOR",FALSE,LOG_LEVEL_INFO);
	t_socket* ptrSocketServer = socketCreate();
	struct sockaddr_in socketInfo;
	int optval = 1;

	if (ptrSocketServer == NULL) {
		log_error(log, "No se pudo crear el socket");
		log_destroy(log);
		return NULL;
	}

	log_info(log, "El socket %d ha sido creado. Puerto %d", ptrSocketServer->descriptor, port);

	//HACER QUE EL SO LIBERE EL PUNTERO INMEDIATAMENTE LUEGO DE CERRAR EL SOCKET
	setsockopt(ptrSocketServer->descriptor, SOL_SOCKET, SO_REUSEADDR, &optval,
			sizeof(optval));

	socketInfo.sin_family = AF_INET;
	socketInfo.sin_addr.s_addr = INADDR_ANY;
	socketInfo.sin_port = htons(port);

	// VINCULAR EL SOCKET CON UNA DIRECCION DE RED ALMACENADA EN 'socketInfo'
	if (bind(ptrSocketServer->descriptor, (struct sockaddr*) &socketInfo,
			sizeof(socketInfo)) != 0) {
		log_error(log, "No se pudo realizar el bind. Descriptor: %d, Puerto: %d", ptrSocketServer->descriptor, port);
		free(ptrSocketServer);
		log_destroy(log);
		return NULL;

	}
	// Se comienza a atender llamadas del cliente
	if (listen(ptrSocketServer->descriptor, MAX_CONEXIONES) == -1) {

		log_error(log, "Hubo un error al realizar el listen. Descriptor: %d, Puerto: %d", ptrSocketServer->descriptor, port);
		log_destroy(log);
		return NULL;
	}

	log_destroy(log);

	return ptrSocketServer;
}

/*
 * @NAME: socketAcceptClient
 * @DESC: Acepta una conexion entrante y devuelve el descriptor asignado a ese cliente.
 * @PARAMS:
 *		ptrListenSocket	: El socket que realiza la escucha
 */
t_socket* socketAcceptClient(t_socket* ptrListenSocket) {

	t_log* log = log_create("sockets.log","SOCKET_SERVIDOR",FALSE,LOG_LEVEL_INFO);
	t_socket *ptrSocketClient = (t_socket *) malloc(sizeof(t_socket));
	ptrSocketClient->ptrAddress = (struct sockaddr_in *) malloc(sizeof(struct sockaddr_in));
	Int32U addrlen = sizeof(struct sockaddr_in);
	Int32U newSocketDescriptor;

	//CADA VEZ QUE SE ACEPTA UNA NUEVA CONEXION ENTRANTE, SE GENERA UN NUEVO DESCRIPTOR
	if ((newSocketDescriptor = accept(ptrListenSocket->descriptor,
			(struct sockaddr*) (ptrSocketClient->ptrAddress), (void *) &addrlen))< 0) {

		log_error(log, "El servidor %d no pudo conectarse a un cliente", ptrListenSocket->descriptor);
		log_destroy(log);
		free(ptrSocketClient->ptrAddress);
		free(ptrSocketClient);
		return NULL;
	}
	log_destroy(log);
	ptrSocketClient->descriptor = newSocketDescriptor;
	return ptrSocketClient;
}

