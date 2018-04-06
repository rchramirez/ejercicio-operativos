#ifndef ESTRUCTURAS_H_
#define ESTRUCTURAS_H_

#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <time.h>
#include "socketes_cliente.h"

#define MOVE_UP 25
#define MOVE_DOWN 26
#define MOVE_RIGHT 27
#define MOVE_LEFT 28

typedef struct {
    char* nombre;
    char* simbolo;
    t_list* hojaDeViaje;
    int vidas;
	int reintentos;
	int posX;
	int posY;
	t_list *pokemones;
}t_entrenador;

typedef struct{
	char* nombre;
	t_queue *pokemones;
}t_info_mapa;

typedef struct{
	int port;
	char *ip;
}t_info_conexion;

typedef struct{
	int movimiento;
}t_movimiento;

typedef struct{
	time_t tiempoTotal;
	time_t tiempoBloqueado;
	int cantidadDeadlocks;
	int cantidadMuertes;
}t_monitoreo;

char ENTRENADOR_NOMBRE[128];
char POKEDEX_PATH[128];

t_entrenador *entrenador;
t_log *log_entrenador;
t_monitoreo *monitoreo;
t_socket_client* cliente;
t_config *configEntrenador;

#endif /* ESTRUCTURAS_H_ */
