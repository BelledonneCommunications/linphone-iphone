################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/launcher-tester.c \
../src/p2pproxy.c \
../src/p2pproxy_wrap.c 

OBJS += \
./src/launcher-tester.o \
./src/p2pproxy.o \
./src/p2pproxy_wrap.o 

C_DEPS += \
./src/launcher-tester.d \
./src/p2pproxy.d \
./src/p2pproxy_wrap.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I/usr/lib/jvm/java-6-openjdk/include -O0 -g3 -Wall -c -fmessage-length=0 -ansi -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/p2pproxy.o: ../src/p2pproxy.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I/usr/lib/jvm/java-6-openjdk/include -O2 -g -Wall -c -fmessage-length=0 -ansi -MMD -MP -MF"$(@:%.o=%.d)" -MT"src/p2pproxy.d" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


