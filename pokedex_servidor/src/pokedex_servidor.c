#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/string.h>
#include "pokedex_servidor.h"

void *hiloConexiones(t_socket* cliente) {

	log_serv = log_create("servidor.log","CLIENTE_" + cliente->descriptor,FALSE,LOG_LEVEL_TRACE);

	while(1){

		t_msj_pokedex* mensaje = recibirMsjPokedex(cliente);

		if(mensaje == NULL){
			log_info(log_serv, "El cliente se desconecto");
			log_destroy(log_serv);
			pthread_exit(NULL);
		}

		int accion = mensaje->accion ;
		switch(accion){

			  case READ :{
				  poke_read(cliente, mensaje);
				  break;
			  }
			  case READDIR :{
				  poke_readdir(cliente, mensaje);
				  break;
			  }
			  case GETATTR :{
				  poke_getattr(cliente, mensaje);
				  break;
			  }
			  case MKDIR :{
				  poke_mkdir(cliente, mensaje);
				  break;
			  }
			  case WRITE :{
				  poke_write(cliente, mensaje);
				  break;
			  }
			  case RENAME :{
				  poke_rename(cliente, mensaje);
				  break;
			  }
			  case RM :{
				  poke_rm(cliente, mensaje);
				  break;
			  }
			  case RMDIR :{
				  poke_rmdir(cliente, mensaje);
				  break;
			  }
			  case CREATE :{
				  poke_create(cliente, mensaje);
				  break;
			  }
			  case TRUNCATE :{
				  poke_truncate(cliente, mensaje);
				  break;
			  }
			  case UTIMENS :{
				  poke_utimens(cliente, mensaje);
				  break;
			  }
			  default :{
				 log_error(log_serv, "no existe ningun comando con asociado a ese codigo %d\n", mensaje->accion );
			  }
		}

		free(mensaje->nuevoNombre);
		free(mensaje->path);
		free(mensaje);
	}

	pthread_exit(NULL);
}

void poke_read(t_socket* cliente, t_msj_pokedex* mensaje){

	//Busco si existe el file
	char buff[mensaje->size];
	if(isExist(mensaje->path, REGULAR)){
		//Creo el buffer a partir del size
		log_info(log_serv, "POKEREAD LEE DESDE: %d HASTA %d, SIZE %d", mensaje->off, mensaje->size + mensaje->off, mensaje->size);
		int sizeRead = readFile(mensaje->path, buff, mensaje->size, mensaje->off);
		log_info(log_serv, "POKEREAD LEYO %d", sizeRead);
		enviarMsjRead(cliente, buff, OK, sizeRead);
	}else
		enviarMsjRead(cliente, buff, ERROR, 0);
}
void poke_readdir(t_socket* cliente, t_msj_pokedex* mensaje){

	//Listo los archivos
	t_list *files = getListFiles(mensaje->path);

	if(files == NULL){
		mensaje->accion = ERROR;
		log_info(log_serv, "POKEREADDIR NO HAY FILES");
	}else{
		mensaje->accion = OK;
		//Le mando la cantidad de files que voy a mandar despues
		mensaje->size = list_size(files);
		log_info(log_serv, "POKEREADDIR CANTIDAD DE FILES %d", mensaje->size);
	}

	enviarMsjReaddir(cliente, files, mensaje->accion);

	list_destroy(files);
}

void poke_getattr(t_socket* cliente, t_msj_pokedex* mensaje){
	//Obtengo el file
	osada_file *file = getFile(mensaje->path);
	if(file == NULL){
		mensaje->accion = ERROR;
		log_info(log_serv, "poke_getattr envia mensaje, NO HAY DIR %d, path %s", mensaje->accion, (char *) mensaje->path);
	}else{
		mensaje->accion = OK;
		log_info(log_serv, "poke_getattr envia mensaje, %d, path %s", mensaje->accion, (char *) file->fname);
	}
	enviarMsjGetattr(cliente, file, mensaje->accion);
}

void poke_mkdir(t_socket* cliente, t_msj_pokedex* mensaje){
	if(!isExist(mensaje->path, DIRECTORY)){

		mensaje->accion = newOsadaFile(mensaje->path, DIRECTORY);

	}else{
		mensaje->accion = ENOENT;
	}
	enviarMsjPokedex(cliente, mensaje);
}

void poke_write(t_socket* cliente, t_msj_pokedex* mensaje){

	char buffer[mensaje->size];
	//Espero el buffer a escribir
	recibirBuffer(cliente, buffer, mensaje->size);

	//Busco si existe el file a escribir
	if(isExist(mensaje->path, REGULAR)){

		log_info(log_serv, "POKEWRITE ESCRIBE DESDE: %d HASTA %d, SIZE %d", mensaje->off, mensaje->size + mensaje->off, mensaje->size);
		//Hago el write
		mensaje->accion = writeFile(mensaje->path, buffer, mensaje->size, mensaje->off);
		enviarMsjPokedex(cliente, mensaje);
		log_info(log_serv, "poke_write envia mensaje, %d, path %s", mensaje->accion, (char *) mensaje->path);

	}else{
		mensaje->accion = ENOENT;
		enviarMsjPokedex(cliente, mensaje);
	}
}
void poke_rename(t_socket* cliente, t_msj_pokedex* mensaje){

	mensaje->accion = renameFile(mensaje->path,mensaje->nuevoNombre);
	enviarMsjPokedex(cliente, mensaje);
}

void poke_rm(t_socket* cliente, t_msj_pokedex* mensaje){

	mensaje->accion = deleteFile(mensaje->path);
	enviarMsjPokedex(cliente, mensaje);
}

void poke_rmdir(t_socket* cliente, t_msj_pokedex* mensaje){

	mensaje->accion = deleteDirectory(mensaje->path);
	enviarMsjPokedex(cliente, mensaje);
}

void poke_create(t_socket* cliente, t_msj_pokedex* mensaje){

	mensaje->accion = newOsadaFile(mensaje->path, REGULAR);
	enviarMsjPokedex(cliente, mensaje);
}

void poke_truncate(t_socket* cliente, t_msj_pokedex* mensaje){
	mensaje->accion = truncateOsadaFile(mensaje->path, mensaje->size);
	enviarMsjPokedex(cliente, mensaje);
}

void poke_utimens(t_socket* cliente, t_msj_pokedex* mensaje){
	mensaje->accion = utimensOsadaFile(mensaje->path, mensaje->time);
	enviarMsjPokedex(cliente, mensaje);
}
