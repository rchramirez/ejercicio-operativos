/*
 * pokedex_cliente.c
 *
 *  Created on: 18/9/2016
 *      Author: Facundo Borda
 */

#include "pokedex_cliente.h"

//----------------CALLBACKS----------------//

/*
 * @DESC
 *  Esta función va a ser llamada cuando a la biblioteca de FUSE le llegue un pedido
 * para obtener la metadata de un archivo/directorio. Esto puede ser tamaño, tipo,
 * permisos, dueño, etc ...
 *
 * @PARAMETROS
 * 		path - El path es relativo al punto de montaje y es la forma mediante la cual debemos
 * 		       encontrar el archivo o directorio que nos solicitan
 * 		stbuf - Esta estructura es la que debemos completar
 *
 * 	@RETURN
 * 		O archivo/directorio fue encontrado. -ENOENT archivo/directorio no encontrado
 */
static int poke_getattr(const char *path, struct stat *stbuf) {

	int res = 0;
	log_debug(pokedexLog, "poke_getattr: %s", path);
	osada_file *osadaFile = NULL;
	memset(stbuf, 0, sizeof(struct stat));


	//Si path es igual a "/" nos estan pidiendo los atributos del punto de montaje
	if (strcmp(path, "/") == 0) {
		//encontrado = i;
		stbuf->st_mode = 0755 | S_IFDIR;
		stbuf->st_nlink = 2;
		stbuf->st_uid = 1001;
		stbuf->st_gid = 1001;

	} else {

		char *dupPath = strdup(path);
		char *fname = getNombreFile(dupPath);
		free(dupPath);

		if(string_length(fname) > OSADA_FILENAME_LENGTH){
			free(fname);
			return -ENOENT;
		}
		free(fname);

		osadaFile = getOsadaFile(path);

		//stbuf->st_uid = 1001;
		//stbuf->st_gid = 1001;

		if(osadaFile == NULL)
			return -ENOENT;
		if (osadaFile->state == DIRECTORY) {
			stbuf->st_mode = S_IFDIR | 0755;
			stbuf->st_nlink = 2;
			stbuf->st_size = OSADA_BLOCK_SIZE;

		} else if (osadaFile->state == REGULAR) {
			stbuf->st_mode = S_IFREG | 0777;
			stbuf->st_nlink = 1;
			stbuf->st_size = osadaFile->file_size;
		}

		struct timespec tim;
		tim.tv_sec = osadaFile->lastmod;
		tim.tv_nsec = 0;
		stbuf->st_mtim = tim;

		free(osadaFile);
	}
	return res;
}

/*
 * @DESC
 *  Esta función va a ser llamada cuando a la biblioteca de FUSE le llege un pedido
 * para obtener el contenido de un archivo
 *
 * @PARAMETROS
 * 		path - El path es relativo al punto de montaje y es la forma mediante la cual debemos
 * 		       encontrar el archivo o directorio que nos solicitan
 * 		buf - Este es el buffer donde se va a guardar el contenido solicitado
 * 		size - Nos indica cuanto tenemos que leer
 * 		offset - A partir de que posicion del archivo tenemos que leer
 *
 * 	@RETURN
 * 		Si se usa el parametro direct_io los valores de retorno son 0 si  elarchivo fue encontrado
 * 		o -ENOENT si ocurrio un error. Si el parametro direct_io no esta presente se retorna
 * 		la cantidad de bytes leidos o -ENOENT si ocurrio un error.
 */
static int poke_read(const char *path, char *buf, size_t size, off_t offset,
		           struct fuse_file_info *fi) {

	log_debug(pokedexLog, "poke_read: %s", path);
	int res = 0;
	//controlar que sea un file y no un directorio
	char *dupPath = strdup(path);
	if (string_ends_with(dupPath, "/")){
		free(dupPath);
		res = -ENOENT;
	}else{
		free(dupPath);
		res = readOsadaFile(path, buf, offset, size);
		log_info(pokedexLog, "Se leyó el archivo: %s", path);
	}
	return res;
}
// Elimina el directorio dado. Esto debería tener éxito sólo si el directorio está vacío (excepto para "." Y "..")
static int poke_rmdir (const char *path) {

	int res = 0;
	log_debug(pokedexLog, "poke_rmdir: %s", path);

	int borrado = eliminaOsadaFile(path, DIRECTORY);
	if (borrado != OK){
		res = -borrado;
	}

	return res;
}

int poke_truncate (const char *path, off_t offset) {
	//se crea un log para saber que paso por ahi
	log_debug(pokedexLog, "accediendo poke_truncate: %s", path);
	char *dupPath = strdup(path);
	char *fname = getNombreFile(dupPath);
	free(dupPath);
	if(string_length(fname) > OSADA_FILENAME_LENGTH){
		log_error(pokedexLog, "El nombre del directorio es mas largo del permitido");
		free(fname);
		return -ENAMETOOLONG;
	}
	free(fname);
	return truncateOsadaFile(path, offset);
}

int poke_flush (const char *path, struct fuse_file_info *fi) {
	return 0;
}

int poke_getxattr (const char *path, const char *name, char *value, size_t size) {
	return 0;
}

static int poke_mkdir (const char *path, mode_t mode) {
	log_debug(pokedexLog, "accediendo poke_mkdir: %s", path);

	char *dupPath = strdup(path);
	char *fname = getNombreFile(dupPath);
	free(dupPath);

	if(string_length(fname) > OSADA_FILENAME_LENGTH){
		log_error(pokedexLog, "El nombre del directorio es mas largo del permitido");
		return -ENAMETOOLONG;
	}

	//Verifico que sea un nombre permitido (no tenga un '.')
	if(string_contains(fname, '.') || string_contains(fname, ','))
		return -ENOTDIR;

	// crear osadaFile que representaria al nuevo directorio
	int res = crearOsadaDirectory(path);
	if (res != OK){
		res = -ENOENT;
	}

	return 0;
}

/*
 * @DESC
 *  Esta función va a ser llamada cuando a la biblioteca de FUSE le llege un pedido
 * para obtener la lista de archivos o directorios que se encuentra dentro de un directorio
 *
 * @PARAMETROS
 * 		path - El path es relativo al punto de montaje y es la forma mediante la cual debemos
 * 		       encontrar el archivo o directorio que nos solicitan
 * 		buf - Este es un buffer donde se colocaran los nombres de los archivos y directorios
 * 		      que esten dentro del directorio indicado por el path
 * 		filler - Este es un puntero a una función, la cual sabe como guardar una cadena dentro
 * 		         del campo buf
 *
 * 	@RETURN
 * 		O directorio fue encontrado. -ENOENT directorio no encontrado
 */
static int poke_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {

	log_debug(pokedexLog, "iniciando poke_readdir: %s", path);
	struct stat *stbuf;

	t_list* lista = listaOsadaFile(path);
	//si la lista esta vacia me devuelve NULL
	if (list_is_empty(lista)){
		list_destroy(lista);
		return -ENOENT;
	}
	void llenarBuffer(void* value){
		osada_file* osadaFile = (osada_file*) value;
		stbuf = malloc(sizeof(struct stat));
		memset(stbuf, 0, sizeof(struct stat));

		/*filler(buf, ".", NULL, 0);
		filler(buf, "..", NULL, 0);*/
		stbuf->st_mtim.tv_sec = osadaFile->lastmod;
		//Ver este tema! sirve?
		//stbuf->st_uid = 1001;
		//stbuf->st_gid = 1001;
		if (osadaFile->state == DIRECTORY) {
			stbuf->st_mode = 0755|S_IFDIR;
			stbuf->st_nlink = 2;
		} else if (osadaFile->state == REGULAR) {
			stbuf->st_mode = 0444 | S_IFREG;
			stbuf->st_nlink = 1;
			stbuf->st_size = osadaFile->file_size;
		}

		//log_debug(pokedexLog, "fname %s", (char *) osadaFile->fname);
		filler(buf, (char *) osadaFile->fname, stbuf, 0);
		free(stbuf);
	}

	list_iterate(lista,llenarBuffer);

	log_debug(pokedexLog, "poke_readdir termino lista: %d", list_size(lista));

	list_destroy_and_destroy_elements(lista, free);

	return 0;
}

static int poke_write (const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {

	int modificado = 0;
	int res = size;
	log_debug(pokedexLog, "poke_write: %s", path);

	char *dupPath = strdup(path);
	//controlar que sea un file y no un directorio
	if (string_ends_with(dupPath, "/")){
		free(dupPath);
		return -ENOENT;
	}
	free(dupPath);
	modificado = writeOsadaFile(path, buf, offset, size);

	if(modificado == OK){

		log_debug(pokedexLog, "Se modificó el archivo: %s", path);
		res = size;
	}else{

		log_debug(pokedexLog, "No se pudo modificar el archivo: %s", path);
		res = -modificado;
	}
	return res;

}

static int poke_open(const char *path, struct fuse_file_info *fi){

	log_debug(pokedexLog, "Se abrió el archivo: %s", path);

	return 0;

}

int poke_rename (const char *path, const char *newPath){

	int res = 0;
	log_debug(pokedexLog, "Se va a renombrar el archivo: %s de la sig manera: %s", path, newPath);


	//controlar que sea un file y no un directorio
	if (string_ends_with(strdup(path),"/")){
		return -EISDIR;
	}

	char *fname = getNombreFile(strdup(newPath));

	if(string_length(fname) > OSADA_FILENAME_LENGTH){
		log_error(pokedexLog, "El nombre del archivo es mas largo del permitido");
		free(fname);
		return -ENAMETOOLONG;
	}

	free(fname);

	int modificado = renameOsadaFile(path, newPath);

	if(modificado == OK){
		log_debug(pokedexLog, "Se renombró el archivo: %s -> %s", path, newPath);
		//return 0;
	}else{
		log_debug(pokedexLog, "No se pudo renombrar el archivo: %s", path);
		res = -modificado;
	}

	return res;

}

static int poke_create (const char *path, mode_t mode, struct fuse_file_info *fi){

	int res = 0;
	log_debug(pokedexLog, "Se quiere crear el archivo: %s", path);

	char *dupPath = strdup(path);
	char *fname = getNombreFile(dupPath);
	free(dupPath);

	if(string_length(fname) > OSADA_FILENAME_LENGTH){
		log_error(pokedexLog, "El nombre del archivo es mas largo del permitido");
		free(fname);
		return -ENAMETOOLONG;
	}
	free(fname);

	int creado = crearOsadaFile(path);

	if(creado == OK){
		log_debug(pokedexLog, "Archivo creado: %s", path);
	}else{
		log_debug(pokedexLog, "No se pudo crear el archivo: %s", path);
		res = -creado;
	}
	return res;
}

int poke_unlink (const char *path){

	int res = 0;
	log_debug(pokedexLog, "Se eliminará el archivo: %s", path);

	int borrado = eliminaOsadaFile(path, REGULAR);

	if(borrado == OK){
		log_debug(pokedexLog, "Archivo eliminado: %s", path);
	}else{
		log_debug(pokedexLog, "No se pudo eliminar el archivo: %s", path);
		res = -borrado;
	}
	return res;

}

int poke_utimens(const char* path, const struct timespec ts[2]){

	int res = 0;
	log_debug(pokedexLog, "Se cambiara el tiempo de modificacion del archivo: %s", path);

	int modificado = utimensOsadaFile(path, ts[0]);

	if(modificado == OK){
		log_debug(pokedexLog, "Archivo modificado: %s", path);
	}else{
		log_debug(pokedexLog, "No se pudo modificar el archivo: %s", path);
		res = -modificado;
	}
	return res;
}

//--------Funciones Auxiliares---------//

/*
 * @DESC
 * Esta función va a ser llamada por poke_read para pedirle al servidor
 * los bytes desde inicio hasta fin del file ubicado en path.
 */

int readOsadaFile(const char *path, char *buf, off_t inicio, size_t size) {

	// enviar mensaje al servidor pidiendo los bytes desde inicio hasta fin
	t_msj_pokedex *msj_read = malloc(sizeof(t_msj_pokedex));

	msj_read->accion = READ;
	msj_read->path = malloc(sizeof(char) * strlen(path) + 1);
	strcpy(msj_read->path, path);
	msj_read->nuevoNombre = string_new();
	msj_read->off = inicio;
	msj_read->size = size;
	msj_read->time = 0;

	enviarMsjPokedex(pokedexServer->ptrSocket, msj_read);

	free(msj_read->nuevoNombre);
	free(msj_read->path);
	free(msj_read);

	//Recibo el mensaje read, con el size leido
	t_msj_read *res = recibirMsjRead(pokedexServer->ptrSocket, buf);

	if(res->res == OK){

		int sizeRead = res->sizeRead;
		free(res);
		return sizeRead;
	}else
		return -ENOENT;
}

/*
 * @DESC
 * Esta función va a ser llamada por poke_getattr para pedirle al servidor
 * la informacion sobre el file que se encuentra ubicado en el path.
 */
osada_file *getOsadaFile(const char *path){

	t_msj_pokedex *msj_getattr = malloc(sizeof(t_msj_pokedex));

	msj_getattr->accion = GETATTR;
	msj_getattr->path = malloc(sizeof(char) * strlen(path) + 1);
	strcpy(msj_getattr->path, path);
	msj_getattr->nuevoNombre = string_new();
	msj_getattr->off = 0;
	msj_getattr->size = 0;
	msj_getattr->time = 0;

	log_info(pokedexLog, "Envio GETATTR");
	enviarMsjPokedex(pokedexServer->ptrSocket, msj_getattr);

	free(msj_getattr->nuevoNombre);
	free(msj_getattr->path);
	free(msj_getattr);

	//falta recibir el file
	t_msj_getattr *res_getattr = recibirMsjGetattr(pokedexServer->ptrSocket);
	log_info(pokedexLog, "recibo GETATTR");
	if(res_getattr != NULL && res_getattr->res == OK){
		log_info(pokedexLog, "recibo GETATTR file");
		osada_file *file = res_getattr->file;
		free(res_getattr);
		return file;
	}else{
		if(res_getattr != NULL){
			free(res_getattr);
		}
		return NULL;
	}
}
/*
 * @DESC
 * Esta función va a ser llamada por poke_readdir para pedirle al servidor
 * un listado con el contenido del directorio que se encuentra ubicado en el path.
 */

t_list*  listaOsadaFile(const char *path){

	t_msj_pokedex *msj_readdir = malloc(sizeof(t_msj_pokedex));

	msj_readdir->accion = READDIR;
	msj_readdir->path = malloc(sizeof(char) * strlen(path) + 1);
	strcpy(msj_readdir->path, path);
	msj_readdir->nuevoNombre = string_new();
	msj_readdir->size = 0;
	msj_readdir->off = 0;
	msj_readdir->time = 0;

	log_info(pokedexLog, "Envio READDIR");
	enviarMsjPokedex(pokedexServer->ptrSocket, msj_readdir);

	free(msj_readdir->nuevoNombre);
	free(msj_readdir->path);
	free(msj_readdir);

	//falta recibir el file
	t_msj_readdir *res = recibirMsjReaddir(pokedexServer->ptrSocket);

	log_info(pokedexLog, "recibo READDIR");
	if(res->res == OK){
		log_info(pokedexLog, "recibi lista osada, cantidad %d", list_size(res->list));
		t_list *list = res->list;
		free(res);
		return list;
	}else{
		return NULL;
	}

}

/*
 * @DESC
 * Esta función va a ser llamada por poke_rmdir para pedirle al servidor
 *  el directorio que se solicita eliminar el directorio ubicado en el path.
 */

int eliminaOsadaFile(const char *path, int tipo){

	t_msj_pokedex *msj_rmdir = malloc(sizeof(t_msj_pokedex));

	if(tipo == REGULAR)
		msj_rmdir->accion = RM;
	else
		msj_rmdir->accion = RMDIR;
	msj_rmdir->path = malloc(sizeof(char) * strlen(path) + 1);
	strcpy(msj_rmdir->path, path);
	msj_rmdir->nuevoNombre = string_new();

	enviarMsjPokedex(pokedexServer->ptrSocket, msj_rmdir);

	free(msj_rmdir->nuevoNombre);
	free(msj_rmdir->path);
	free(msj_rmdir);
	//falta recibir el file
	msj_rmdir = recibirMsjPokedex(pokedexServer->ptrSocket);
	int accion = msj_rmdir->accion;
	free(msj_rmdir->nuevoNombre);
	free(msj_rmdir->path);
	free(msj_rmdir);

	return accion;
}

/*
 * @DESC
 * Esta función va a ser llamada por poke_mkdir para pedirle al servidor
 * que se cree un nuevo directorio a traves del path enviado.
 */
int crearOsadaDirectory(const char *path){

	t_msj_pokedex *msj_mkdir = malloc(sizeof(t_msj_pokedex));

	msj_mkdir->accion = MKDIR;
	msj_mkdir->path = malloc(sizeof(char) * strlen(path) + 1);
	strcpy(msj_mkdir->path, path);
	msj_mkdir->nuevoNombre = string_new();
	msj_mkdir->off = 0;
	msj_mkdir->size = 0;
	msj_mkdir->time = 0;

	enviarMsjPokedex(pokedexServer->ptrSocket, msj_mkdir);

	free(msj_mkdir->nuevoNombre);
	free(msj_mkdir->path);
	free(msj_mkdir);
	//falta recibir el nuevofile
	msj_mkdir = recibirMsjPokedex(pokedexServer->ptrSocket);
	int accion = msj_mkdir->accion;
	free(msj_mkdir->nuevoNombre);
	free(msj_mkdir->path);
	free(msj_mkdir);

	return accion;
}

/*
 * @DESC
 * Esta función va a ser llamada por poke_create para pedirle al servidor
 * que se cree un nuevo directorio a traves del path enviado.
 */
int crearOsadaFile(const char *path){

	t_msj_pokedex *msj_create = malloc(sizeof(t_msj_pokedex));

	msj_create->accion = CREATE;
	msj_create->path = malloc(sizeof(char) * strlen(path) + 1);
	strcpy(msj_create->path, path);
	msj_create->nuevoNombre = string_new();
	msj_create->off = 0;
	msj_create->size = 0;
	msj_create->time = 0;

	enviarMsjPokedex(pokedexServer->ptrSocket, msj_create);

	free(msj_create->nuevoNombre);
	free(msj_create->path);
	free(msj_create);
	//falta recibir el nuevofile
	msj_create = recibirMsjPokedex(pokedexServer->ptrSocket);
	int accion = msj_create->accion;
	free(msj_create->nuevoNombre);
	free(msj_create->path);
	free(msj_create);

	return accion;
}

int writeOsadaFile(const char *path, const char *buf, off_t offset, size_t size){

	t_msj_pokedex *msj_write = malloc(sizeof(t_msj_pokedex));

	msj_write->accion = WRITE;
	msj_write->path = malloc(sizeof(char) * strlen(path) + 1);
	strcpy(msj_write->path, path);
	msj_write->nuevoNombre = string_new();
	msj_write->off = offset;
	msj_write->size = size;
	msj_write->time = 0;

	enviarMsjPokedex(pokedexServer->ptrSocket, msj_write);

	free(msj_write->nuevoNombre);
	free(msj_write->path);
	free(msj_write);
	char nuevoBuf[size];
	memcpy(nuevoBuf, buf, size);

	enviarBuffer(pokedexServer->ptrSocket, nuevoBuf, size);

	msj_write = recibirMsjPokedex(pokedexServer->ptrSocket);
	int accion = msj_write->accion;
	free(msj_write->nuevoNombre);
	free(msj_write->path);
	free(msj_write);

	return accion;
}

/*
 * @DESC
 * Esta función va a ser llamada por poke_rename para pedirle al servidor
 * que se cambie el nombre del file y ademas el path de origen inclusive.
 */
int renameOsadaFile(const char *path, const char *newPath){

	t_msj_pokedex *msj_rename_ori = malloc(sizeof(t_msj_pokedex));
	t_msj_pokedex *msj_rename_des;

	msj_rename_ori->accion = RENAME;
	msj_rename_ori->path = malloc(sizeof(char) * strlen(path) + 1);
	strcpy(msj_rename_ori->path, path);
	msj_rename_ori->nuevoNombre = malloc(sizeof(char) * strlen(newPath) + 1);
	strcpy(msj_rename_ori->nuevoNombre, newPath);
	msj_rename_ori->off = 0;
	msj_rename_ori->size = 0;
	msj_rename_ori->time = 0;

	enviarMsjPokedex(pokedexServer->ptrSocket, msj_rename_ori);

	free(msj_rename_ori->nuevoNombre);
	free(msj_rename_ori->path);
	free(msj_rename_ori);

	// hay que enviar el otro path, donde lo envio?
	msj_rename_des = recibirMsjPokedex(pokedexServer->ptrSocket);

	int accion = msj_rename_des->accion;
	free(msj_rename_des->nuevoNombre);
	free(msj_rename_des->path);
	free(msj_rename_des);

	return accion;
}

int truncateOsadaFile(const char *path, off_t size){

	t_msj_pokedex *msj_truncate = malloc(sizeof(t_msj_pokedex));

	msj_truncate->accion = TRUNCATE;
	msj_truncate->path = malloc(sizeof(char) * strlen(path) + 1);
	strcpy(msj_truncate->path, path);
	msj_truncate->nuevoNombre = string_new();
	msj_truncate->off = 0;
	msj_truncate->size = size;
	msj_truncate->time = 0;

	enviarMsjPokedex(pokedexServer->ptrSocket, msj_truncate);

	free(msj_truncate->nuevoNombre);
	free(msj_truncate->path);
	free(msj_truncate);
	msj_truncate = recibirMsjPokedex(pokedexServer->ptrSocket);

	int accion = msj_truncate->accion;
	free(msj_truncate->nuevoNombre);
	free(msj_truncate->path);
	free(msj_truncate);

	if(accion == OK)
		return 0;
	else
		return -accion;
}

int utimensOsadaFile(const char* path, const struct timespec ts){

	t_msj_pokedex *msj_ultimens = malloc(sizeof(t_msj_pokedex));

	msj_ultimens->accion = UTIMENS;
	msj_ultimens->path = malloc(sizeof(char) * strlen(path) + 1);
	strcpy(msj_ultimens->path, path);
	msj_ultimens->nuevoNombre = string_new();
	msj_ultimens->off = 0;
	msj_ultimens->size = 0;
	msj_ultimens->time = (long) ts.tv_sec;

	enviarMsjPokedex(pokedexServer->ptrSocket, msj_ultimens);

	free(msj_ultimens->nuevoNombre);
	free(msj_ultimens->path);
	free(msj_ultimens);
	msj_ultimens = recibirMsjPokedex(pokedexServer->ptrSocket);
	int accion = msj_ultimens->accion;
	free(msj_ultimens->nuevoNombre);
	free(msj_ultimens->path);
	free(msj_ultimens);

	if(accion == OK)
		return 0;
	else
		return -accion;
}

char *getNombreFile(char *path){

	char *nombreFile = string_new();
	char** subpath = string_split(path, "/");

	void closure(char *value){
		free(nombreFile);
		nombreFile = value;
	}

	string_iterate_lines(subpath, closure);

	free(subpath);

	if(string_ends_with(nombreFile, "/")){

		char *subnombreFile = string_substring_until(nombreFile, string_length(nombreFile) - 1);
		free(nombreFile);
		nombreFile = subnombreFile;
	}

	return nombreFile;
}

bool string_contains(char *path, char caracter){

	char *c;

	 c = strchr (path, caracter);

	 return c != NULL;
}

 /*
 * Esta es la estructura principal de FUSE con la cual nosotros le decimos a
 * biblioteca que funciones tiene que invocar segun que se le pida a FUSE.
 * Como se observa la estructura contiene punteros a funciones.
 */


static struct fuse_operations poke_oper = {
		.getattr = poke_getattr,
		.getxattr = poke_getxattr,
		.readdir = poke_readdir,
		.read = poke_read,
		.rmdir = poke_rmdir,
		.open = poke_open,
		.mkdir = poke_mkdir,
		.write = poke_write,
		.truncate = poke_truncate,
		.rename = poke_rename,
		.unlink = poke_unlink,
		.flush = poke_flush,
		.create = poke_create,
		.utimens = poke_utimens
};

/*
 *  Dentro de los argumentos que recibe nuestro programa obligatoriamente
 * debe estar el path al directorio donde vamos a montar nuestro FS
 *
 * ARGS: PATH - IP - PORT
 */

int main(int argc, char *argv[]) {

	char *ip = getenv("IP_POKEDEX"); //"127.0.0.1"
	int puerto = atoi(getenv("PUERTO_POKEDEX"));//8080

	//Me conecto al pokedex_servidor
	pokedexServer = socketCreateClientAndConnect(ip, puerto); //ip y port se obtienen por variable de entorno

	if (pokedexServer == NULL) {
		printf("Hubo un error al conectarme al servidor en IP = %s y PUERTO = %d",ip, puerto);
		return EXIT_FAILURE;
	}
	char *nombreLog = string_from_format("pokedex_cliente_%d.log", pokedexServer->ptrSocket->descriptor);

	pokedexLog = log_create(nombreLog, "POKEDEX_CLIENTE", 1,
			LOG_LEVEL_DEBUG);
	log_info(pokedexLog, "Me conecte al servidor. IP = %s PUERTO = %d", ip,
			puerto);
	free(nombreLog);

	return fuse_main(argc, argv, &poke_oper, NULL);
}
