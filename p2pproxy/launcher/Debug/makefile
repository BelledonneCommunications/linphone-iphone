################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir.mk
-include src/subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: p2pproxy-launcher

# Tool invocations
p2pproxy-launcher: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C Linker'
	gcc -L/usr/lib/jvm/java-6-openjdk/jre/lib/amd64 -L/usr/lib/jvm/java-6-openjdk/jre/lib/i386 -L/usr/lib/jvm/java-6-openjdk/jre/lib/amd64/server -L/usr/lib/jvm/java-6-openjdk/jre/lib/i386/server -o"p2pproxy-launcher" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(OBJS)$(EXECUTABLES)$(C_DEPS) p2pproxy-launcher
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
