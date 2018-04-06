cd ..
dir=`pwd`
for x in Sockets nivel-gui mapa entrenador pokedex_cliente pokedex_servidor
do
  cd ${x}/Debug/
  make clean
  make
  cd ${dir}
done
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${dir}/Sockets/Debug/
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${dir}/nivel-gui/Debug/
while :
do
  clear
  echo -n "Ingrese NOMBRE del archivo log: "
  read nombre
  echo "MENU DE OPCIONES"
  echo "1) Instalar Bibliotecas"
  echo "2) Proceso Mapa"
  echo "3) Proceso Entrenador"
  echo "4) Proceso Pokedex Cliente"
  echo "5) Proceso Pokedex Servidor"
  echo "6) Salir"
  echo -n "Ingrese OPCION[1-6]: "
  read opc
  case $opc in
    1) echo "Instalando Bibliotecas...";
    ./instalar.sh;;
    2) cd mapa/Debug/;
    echo -n "Ingrese MAPA: ";
    read mapa;
    echo -n "Ingrese PATH: ";
    read path;
    valgrind --log-file=${dir}/$nombre.txt --leak-check=yes ./mapa $mapa $path;
    echo "Presione una tecla para continuar...";
    read tecla;
    cd ${dir};;
    3) cd entrenador/Debug/;
    echo -n "Ingrese ENTRENADOR: ";
    read entrenador;
    echo -n "Ingrese PATH: ";
    read path;
    valgrind --log-file=${dir}/$nombre.txt --leak-check=yes ./entrenador $entrenador $path;
    echo "Presione una tecla para continuar...";
    read tecla;
    cd ${dir};;
    4) cd pokedex_cliente/Debug/;
    echo -n "Ingrese IP: ";
    read ip;
    export IP_POKEDEX=$ip;
    echo -n "Ingrese PUERTO: ";
    read puerto;
    export PUERTO_POKEDEX=$puerto;
    echo -n "Ingrese PATH: ";
    read path;
    valgrind --log-file=${dir}/$nombre.txt --leak-check=yes ./pokedex_cliente $path -s -d;
    echo "Presione una tecla para continuar...";
    read tecla;
    cd ${dir};;
    5) cd pokedex_servidor/Debug/;
    echo -n "Ingrese PUERTO: ";
    read puerto;
    echo -n "Ingrese PATH: ";
    read path;
    valgrind --log-file=${dir}/$nombre.txt --leak-check=yes ./pokedex_servidor $puerto $path;
    echo "Presione una tecla para continuar...";
    read tecla;
    cd ${dir};;
    6) echo "Script VALGRIND Finalizado!!!";
    exit 1;;
    *) echo "Opcion Incorrecta";
    echo "Presione una tecla para continuar...";
    read tecla;;
  esac
done
