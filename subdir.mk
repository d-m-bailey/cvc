################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_UPPER_SRCS += \
../src/gzstream.C 

CC_SRCS += \
../src/CCdlParserDriver.cc \
../src/CCircuit.cc \
../src/CCondition.cc \
../src/CConnection.cc \
../src/CConnectionCount.cc \
../src/CCvcDb.cc \
../src/CCvcDb_error.cc \
../src/CCvcDb_init.cc \
../src/CCvcDb_interactive.cc \
../src/CCvcDb_main.cc \
../src/CCvcDb_print.cc \
../src/CCvcDb_utility.cc \
../src/CCvcParameters.cc \
../src/CDevice.cc \
../src/CEventQueue.cc \
../src/CFixedText.cc \
../src/CInstance.cc \
../src/CModel.cc \
../src/CNormalValue.cc \
../src/CParameterMap.cc \
../src/CPower.cc \
../src/CSet.cc \
../src/CVirtualNet.cc \
../src/CvcMaps.cc \
../src/cdlParser.cc \
../src/cdlScanner.cc \
../src/cvc.cc \
../src/resource.cc \
../src/utility.cc 

C_SRCS += \
../src/exitfail.c \
../src/obstack.c 

CC_DEPS += \
./src/CCdlParserDriver.d \
./src/CCircuit.d \
./src/CCondition.d \
./src/CConnection.d \
./src/CConnectionCount.d \
./src/CCvcDb.d \
./src/CCvcDb_error.d \
./src/CCvcDb_init.d \
./src/CCvcDb_interactive.d \
./src/CCvcDb_main.d \
./src/CCvcDb_print.d \
./src/CCvcDb_utility.d \
./src/CCvcParameters.d \
./src/CDevice.d \
./src/CEventQueue.d \
./src/CFixedText.d \
./src/CInstance.d \
./src/CModel.d \
./src/CNormalValue.d \
./src/CParameterMap.d \
./src/CPower.d \
./src/CSet.d \
./src/CVirtualNet.d \
./src/CvcMaps.d \
./src/cdlParser.d \
./src/cdlScanner.d \
./src/cvc.d \
./src/resource.d \
./src/utility.d 

OBJS += \
./src/CCdlParserDriver.o \
./src/CCircuit.o \
./src/CCondition.o \
./src/CConnection.o \
./src/CConnectionCount.o \
./src/CCvcDb.o \
./src/CCvcDb_error.o \
./src/CCvcDb_init.o \
./src/CCvcDb_interactive.o \
./src/CCvcDb_main.o \
./src/CCvcDb_print.o \
./src/CCvcDb_utility.o \
./src/CCvcParameters.o \
./src/CDevice.o \
./src/CEventQueue.o \
./src/CFixedText.o \
./src/CInstance.o \
./src/CModel.o \
./src/CNormalValue.o \
./src/CParameterMap.o \
./src/CPower.o \
./src/CSet.o \
./src/CVirtualNet.o \
./src/CvcMaps.o \
./src/cdlParser.o \
./src/cdlScanner.o \
./src/cvc.o \
./src/exitfail.o \
./src/gzstream.o \
./src/obstack.o \
./src/resource.o \
./src/utility.o 

C_UPPER_DEPS += \
./src/gzstream.d 

C_DEPS += \
./src/exitfail.d \
./src/obstack.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: Cygwin C++ Compiler'
	g++ -I"C:\cygwin\usr\include" -O0 -g3 -Wall -c -fmessage-length=0 -std=gnu++11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cygwin C Compiler'
	gcc -I"C:\cygwin\usr\include" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/%.o: ../src/%.C
	@echo 'Building file: $<'
	@echo 'Invoking: Cygwin C++ Compiler'
	g++ -I"C:\cygwin\usr\include" -O0 -g3 -Wall -c -fmessage-length=0 -std=gnu++11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


