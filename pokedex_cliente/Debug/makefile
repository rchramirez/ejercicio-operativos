################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables

# All Target
all: pokedex_cliente

dependents:
	-cd $(PWD)/../../Sockets/Debug && $(MAKE) all
	-cd $(PWD)/../../mapa/Debug && $(MAKE) all
	-cd $(PWD)/../../entrenador/Debug && $(MAKE) all
	-cd $(PWD)/../../nivel-gui/Debug && $(MAKE) all

# Tool invocations
pokedex_cliente: $(OBJS) $(USER_OBJS) $(PWD)/../../Sockets/Debug/libSockets.so $(PWD)/../../mapa/Debug/mapa $(PWD)/../../entrenador/Debug/entrenador $(PWD)/../../nivel-gui/Debug/libnivel-gui.so
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C Linker'
	gcc -L $(PWD)/../../nivel-gui/Debug -L $(PWD)/../../Sockets/Debug -o "pokedex_cliente" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(OBJS)$(C_DEPS)$(EXECUTABLES) pokedex_cliente
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:
$(PWD)/../../Sockets/Debug/libSockets.so:
$(PWD)/../../mapa/Debug/mapa:
$(PWD)/../../entrenador/Debug/entrenador:
$(PWD)/../../nivel-gui/Debug/libnivel-gui.so:

-include ../makefile.targets
