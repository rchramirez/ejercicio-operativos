################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../socketes.c \
../socketes_cliente.c \
../socketes_servidor.c 

OBJS += \
./socketes.o \
./socketes_cliente.o \
./socketes_servidor.o 

C_DEPS += \
./socketes.d \
./socketes_cliente.d \
./socketes_servidor.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


