#!/bin/bash
function instalar(){
	./instalar.sh
}
# Funcion Compilar
function compilar(){
	clear
	cd ..
	dir=`pwd`
	pwd
	for x in Sockets pokedex_servidor
	do
		cd ${x}/Debug/
		make clean
		make all
		cd ${dir}
	done
	cd pokedex_servidor/Debug
	echo "Haciendo el EXPORT.."
	pwd
	echo "ANTES del EXPORT.."
	ldd pokedex_servidor
	export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${dir}/Sockets/Debug/
	echo "DESPUES del EXPORT.."
	ldd pokedex_servidor
	cd ${dir}/scripts/
	echo "Compilacion Completa!!"
}
# Funcion Ejecutar
function ejecutar(){
	clear
	dir=`pwd`
	cd ../pokedex_servidor/Debug/
	echo -n "Ingrese PUERTO:"
	read PUERTO
	echo -n "Ingrese PATH:"
	read PATH
	./pokedex_servidor $PUERTO $PATH
	cd ${dir}
	echo "Ejecucion Completa!!"
}
#Menu Principal
clear
while :
do
	echo " PROCESO POKEDEX SERVIDOR "
	echo "1. Instalar Todo"
	echo "2. Compilar y Ejecutar"
	echo "3. Salir"
	echo -n "Seleccione una opcion [1 - 3]"
	read opcion
	case $opcion in
	1) echo "INSTALANDO BIBLIOTECAS...";
	instalar;;
	2) echo "COMPILANDO y EJECUTANDO";
	compilar;
	ejecutar;;
	3) echo "Script POKEDEX SERVIDOR Finalizado!!!";
	exit 1;;
	*) echo "Intentelo nuevamente!!";
	echo "Presiona una tecla para continuar...";
	read foo;;
	esac
done
exit
