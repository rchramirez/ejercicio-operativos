/*
 * socketes_cliente.c
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
#include "socketes_cliente.h"

t_socket* socketCreate();
Boolean socketConnect(t_socket_client *ptrSocketClient, String ptrServerAddress, Int32U serverPort);
t_socket* socketGetServerFromAddress(struct sockaddr_in socketAddress);


/**
 * @NAME: socketCreateClientAndConnect
 * @DESC: Crea un socket para ser utilizado como Cliente y lo conecta a un servidor.
 * Devuelve un struct con descriptor correspondiente.
 */
t_socket_client* socketCreateClientAndConnect(String ptrServerAddress, Int32U serverPort) {

	t_log* log = log_create("sockets.log","SOCKET_CLIENTE",FALSE,LOG_LEVEL_INFO);
	t_socket* ptrNewSocket = socketCreate();

	if (ptrNewSocket == NULL) {
		log_error(log, "No se pudo crear el socket");
		return NULL;
	}

	log_info(log, "El socket %d ha sido creado", ptrNewSocket->descriptor);

	t_socket_client *ptrSocketClient = (t_socket_client *) malloc(sizeof(t_socket_client));

	ptrSocketClient->ptrSocket = ptrNewSocket;

	if(socketConnect(ptrSocketClient, ptrServerAddress, serverPort)){
		log_info(log, "El socket %d ha sido conectado al puerto %d", ptrNewSocket->descriptor, serverPort);
		log_destroy(log);
		return ptrSocketClient;
	}else{
		log_error(log, "No se pudo conectar el socket: %d al puerto %d", ptrNewSocket->descriptor, serverPort);
		log_destroy(log);
		free(ptrSocketClient->ptrSocket);
		free(ptrSocketClient);
		return NULL;
	}

}

/*
 * @NAME: socketConnect
 * @DESC: Conecta al socket cliente al servidor a traves de una Direccion IP y un Puerto.
 * Devuelve TRUE en caso de exito, FALSE caso contrario
 * @PARAMS:
 *		ptrSocketClient		: Socket cliente
 *		ptrServerAddress	: Direccion IP del server al que desea conectarse
 *		serverPort			: Puerto del server al que desea conectarse
 */
Boolean socketConnect(t_socket_client *ptrSocketClient, String ptrServerAddress,
		Int32U serverPort) {

	struct sockaddr_in socketAddress;

	socketAddress.sin_family = AF_INET;
	socketAddress.sin_addr.s_addr = inet_addr(ptrServerAddress);
	socketAddress.sin_port = htons(serverPort);

	// CONECTA AL SOCKET CON ESA DIRECCION A TRAVES DE ESE PUERTO
	if (connect(ptrSocketClient->ptrSocket->descriptor,
			(struct sockaddr*) &socketAddress, sizeof(socketAddress)) == -1) {
		return FALSE; //false,error value
	}

	ptrSocketClient->ptrSocketServer = socketGetServerFromAddress(
			socketAddress);

	return TRUE;

}
