/*
 * socketes_cliente.h
 *
 *  Created on: 6/9/2016
 *      Author: utnso
 */

#ifndef SOCKETES_CLIENTE_H_
#define SOCKETES_CLIENTE_H_

#include "socketes.h"

//FUNCIONES DE MANEJO DE SOCKETS
//=================================================================

t_socket_client* socketCreateClient();
Boolean socketConnect(t_socket_client *ptrSocketClient, String ptrServerAddress,
		Int32U serverPort);
t_socket_client* socketCreateClientAndConnect(String ptrServerAddress, Int32U serverPort);;

#endif /* SOCKETES_CLIENTE_H_ */
