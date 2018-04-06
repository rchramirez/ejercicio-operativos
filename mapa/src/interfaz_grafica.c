#include "interfaz_grafica.h"

ITEM_NIVEL* _search_item_by_id(t_list* items, char id);

void mostrarPantalla(char* mapa){
	pthread_mutex_lock (&mutexGuiMapa);
	nivel_gui_dibujar(guiMapa, mapa);
	pthread_mutex_unlock (&mutexGuiMapa);
}

void agregarEntrenador(char idEntrenador){
	pthread_mutex_lock (&mutexGuiMapa);
	CrearPersonaje(guiMapa, idEntrenador, 0, 0);
	pthread_mutex_unlock (&mutexGuiMapa);
}

void borrarEntrenador(char idEntrenador){
	pthread_mutex_lock (&mutexGuiMapa);
	BorrarItem(guiMapa, idEntrenador);
	pthread_mutex_unlock (&mutexGuiMapa);
}

void moverEntrenador(char idEntrenador, int movimiento){
	pthread_mutex_lock (&mutexGuiMapa);
	ITEM_NIVEL* item = _search_item_by_id(guiMapa, idEntrenador);
	int posX = item->posx;
	int posY = item->posy;
	switch(movimiento) {
	      case MOVE_RIGHT :
	    	 MoverPersonaje(guiMapa, idEntrenador, posX+1, posY );
	         break;
	      case MOVE_LEFT :
	    	  MoverPersonaje(guiMapa, idEntrenador, posX-1, posY );
	         break;
	      case MOVE_UP :
	    	  MoverPersonaje(guiMapa, idEntrenador, posX, posY+1);
	         break;
	      case MOVE_DOWN :
	    	  MoverPersonaje(guiMapa, idEntrenador, posX, posY-1 );
	         break;
	}
	pthread_mutex_unlock (&mutexGuiMapa);
}

int getDistanciaEntrenador(char idEntrenador){

	int distancia = 0;
	pthread_mutex_lock (&mutexGuiMapa);
	ITEM_NIVEL* item = _search_item_by_id(guiMapa, idEntrenador);
	distancia = item->posx + item->posy;
	pthread_mutex_unlock (&mutexGuiMapa);

	return distancia;
}

void crearPokenest(char* key, void* value){

	t_pokenest *pokenest = (t_pokenest *) value;
	pthread_mutex_lock (&mutexGuiMapa);
	CrearCaja(guiMapa, *pokenest->identificador, pokenest->posicionX, pokenest->posicionY, pokenest->cantidadPokemons);
	pthread_mutex_unlock (&mutexGuiMapa);
}

void ponerEnPokenest(char *idPokemon, int cantidad){
	pthread_mutex_lock (&mutexGuiMapa);
	ITEM_NIVEL* item = _search_item_by_id(guiMapa, *idPokemon);
	item->quantity = item->quantity + cantidad ;
	pthread_mutex_unlock (&mutexGuiMapa);
}

void sacarDePokenest(char *idPokemon){
	pthread_mutex_lock (&mutexGuiMapa);
	restarRecurso(guiMapa, *idPokemon);
	pthread_mutex_unlock (&mutexGuiMapa);
}

void agregarPokenests(){
	//Itero la collection de pokenest y los creo de a uno
	pthread_mutex_lock(&mutexPokenests);
	dictionary_iterator(pokenests,crearPokenest);
	pthread_mutex_unlock(&mutexPokenests);

}

t_posicion *getUbicacionPokenest(char *pokemon){

	t_posicion *posicion = malloc(sizeof(t_posicion));

	pthread_mutex_lock(&mutexPokenests);
	t_pokenest *pokenest = dictionary_get(pokenests, pokemon);
	posicion->posicionX = pokenest->posicionX;
	posicion->posicionY = pokenest->posicionY;
	pthread_mutex_unlock(&mutexPokenests);

	return posicion;
}

int getDistanciaPokenest(char *pokemon){

	int distancia = 0;
	t_posicion *posicion = getUbicacionPokenest(pokemon);
	distancia = posicion->posicionX + posicion->posicionY;
	free(posicion);
	return distancia;
}
