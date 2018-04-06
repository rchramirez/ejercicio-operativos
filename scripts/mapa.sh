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
	for x in Sockets nivel-gui mapa
	do
		cd ${x}/Debug/
		make clean
		make all
		cd ${dir}
	done
	cd mapa/Debug
	echo "Haciendo el EXPORT.."
	pwd
	echo "ANTES del EXPORT.."
	ldd mapa
	export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${dir}/Sockets/Debug/
	export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${dir}/nivel-gui/Debug/
	echo "DESPUES del EXPORT.."
	ldd mapa
	cd ${dir}/scripts/
	echo "Compilacion Completa!!"
}
# Funcion Ejecutar
function ejecutar(){
	clear
	dir=`pwd`
	cd ../mapa/Debug/
	echo -n "Ingrese MAPA:"
	read MAPA
	echo -n "Ingrese PATH:"
	read PATH
	./mapa $MAPA $PATH
	cd ${dir}
	echo "Ejecucion Completa!!"
}
#Menu Principal
clear
while :
do
	echo " PROCESO MAPA "
	echo "1. Instalar Todo"
	echo "2. Compilar y Ejecutar"
	echo "3. Salir"
	echo -n "Seleccione una opcion [1 - 3]"
	read opcion
	case $opcion in
	1) echo "INSTALANDO BIBLIOTECAS...";
	instalar;;
	2) echo "COMPILANDO y EJECUTANDO:";
	compilar;
	ejecutar;;
	3) echo "Script MAPA Finalizado!!!";
	exit 1;;
	*) echo "Intentelo nuevamente!!";
	echo "Presiona una tecla para continuar...";
	read foo;;
	esac
done
exit
