/*
 * socketes_servidor.h
 *
 *  Created on: 6/9/2016
 *      Author: utnso
 */

#ifndef SOCKETES_SERVIDOR_H_
#define SOCKETES_SERVIDOR_H_
#include "socketes.h"


//FUNCIONES DE MANEJO DE SOCKETS
//=================================================================

t_socket* socketCreateServer(Int32U port);
t_socket* socketAcceptClient(t_socket *ptrListenSocket);
Boolean socketListen(t_socket *ptrSocket);

#endif /* SOCKETES_SERVIDOR_H_ */
