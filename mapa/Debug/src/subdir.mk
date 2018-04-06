################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/deadlock.c \
../src/interfaz_grafica.c \
../src/main.c \
../src/mapa.c \
../src/planificador.c \
../src/pokemon_utils.c \
../src/queue_utils.c 

OBJS += \
./src/deadlock.o \
./src/interfaz_grafica.o \
./src/main.o \
./src/mapa.o \
./src/planificador.o \
./src/pokemon_utils.o \
./src/queue_utils.o 

C_DEPS += \
./src/deadlock.d \
./src/interfaz_grafica.d \
./src/main.d \
./src/mapa.d \
./src/planificador.d \
./src/pokemon_utils.d \
./src/queue_utils.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I $(PWD)/../../Sockets -I $(PWD)/../../nivel-gui -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


