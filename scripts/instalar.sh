#!/bin/bash
clear
cd ../..
dir=`pwd`
# Instalando las commons
git clone https://github.com/sisoputnfrba/so-commons-library.git
cd so-commons-library
make
sudo make install
ls -l /usr/include/commons
echo "Se instala la biblioteca Commons con Éxito!!"
cd ${dir}
# Instalando nCurse
sudo apt-get install libncurses5-dev
# Instalando la GUI
git clone https://github.com/sisoputnfrba/so-nivel-gui-library.git
cd so-nivel-gui-library
make
sudo make install
echo "Se instala la biblioteca Nivel GUI con Éxito!!"
cd ${dir}
# Instalando las pkmn
git clone https://github.com/sisoputnfrba/so-pkmn-utils.git
cd so-pkmn-utils/src
make all
ls build/
sudo make install
echo "Se instala la biblioteca PKMN con Éxito!!"
cd ${dir}/tp-2016-2c-Los-Picantes/scripts/
echo "Instalacion Completa!!"
exit
