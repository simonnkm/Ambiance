################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_SRCS += \
../Core/Src/blue_unit_conversion.s 

C_SRCS += \
../Core/Src/BLUETOOTH.c \
../Core/Src/BOARD.c \
../Core/Src/COMM.c \
../Core/Src/DiscountIO.c \
../Core/Src/FIFO.c \
../Core/Src/FLASH.c \
../Core/Src/GPIO.c \
../Core/Src/I2C.c \
../Core/Src/Main.c \
../Core/Src/TIMERS.c \
../Core/Src/UART.c \
../Core/Src/mainunused.c \
../Core/Src/stm32wb0x_hal_msp.c \
../Core/Src/stm32wb0x_it.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_stm32wb0x.c 

OBJS += \
./Core/Src/BLUETOOTH.o \
./Core/Src/BOARD.o \
./Core/Src/COMM.o \
./Core/Src/DiscountIO.o \
./Core/Src/FIFO.o \
./Core/Src/FLASH.o \
./Core/Src/GPIO.o \
./Core/Src/I2C.o \
./Core/Src/Main.o \
./Core/Src/TIMERS.o \
./Core/Src/UART.o \
./Core/Src/blue_unit_conversion.o \
./Core/Src/mainunused.o \
./Core/Src/stm32wb0x_hal_msp.o \
./Core/Src/stm32wb0x_it.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_stm32wb0x.o 

S_DEPS += \
./Core/Src/blue_unit_conversion.d 

C_DEPS += \
./Core/Src/BLUETOOTH.d \
./Core/Src/BOARD.d \
./Core/Src/COMM.d \
./Core/Src/DiscountIO.d \
./Core/Src/FIFO.d \
./Core/Src/FLASH.d \
./Core/Src/GPIO.d \
./Core/Src/I2C.d \
./Core/Src/Main.d \
./Core/Src/TIMERS.d \
./Core/Src/UART.d \
./Core/Src/mainunused.d \
./Core/Src/stm32wb0x_hal_msp.d \
./Core/Src/stm32wb0x_it.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_stm32wb0x.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0plus -std=gnu11 -DUSE_HAL_DRIVER -DSTM32WB05 -DUSE_NUCLEO_64 -c -I../Core/Inc -I../Drivers/STM32WB0x_HAL_Driver/Inc -I../Drivers/STM32WB0x_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32WB0X/Include -I../Drivers/CMSIS/Include -I"C:/Users/stipi/Desktop/HWstuff/ECE129/WildlifeSource/WPSSource/WildlifeSource/Drivers/BSP/STM32WB0x-nucleo" -I../Drivers/BSP/STM32WB0x-nucleo -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Core/Src/%.o: ../Core/Src/%.s Core/Src/subdir.mk
	arm-none-eabi-gcc -mcpu=cortex-m0plus -c -I"C:/Users/stipi/Desktop/HWstuff/ECE129/WildlifeSource/WPSSource/WildlifeSource/Drivers/BSP/STM32WB0x-nucleo" -x assembler-with-cpp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@" "$<"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/BLUETOOTH.cyclo ./Core/Src/BLUETOOTH.d ./Core/Src/BLUETOOTH.o ./Core/Src/BLUETOOTH.su ./Core/Src/BOARD.cyclo ./Core/Src/BOARD.d ./Core/Src/BOARD.o ./Core/Src/BOARD.su ./Core/Src/COMM.cyclo ./Core/Src/COMM.d ./Core/Src/COMM.o ./Core/Src/COMM.su ./Core/Src/DiscountIO.cyclo ./Core/Src/DiscountIO.d ./Core/Src/DiscountIO.o ./Core/Src/DiscountIO.su ./Core/Src/FIFO.cyclo ./Core/Src/FIFO.d ./Core/Src/FIFO.o ./Core/Src/FIFO.su ./Core/Src/FLASH.cyclo ./Core/Src/FLASH.d ./Core/Src/FLASH.o ./Core/Src/FLASH.su ./Core/Src/GPIO.cyclo ./Core/Src/GPIO.d ./Core/Src/GPIO.o ./Core/Src/GPIO.su ./Core/Src/I2C.cyclo ./Core/Src/I2C.d ./Core/Src/I2C.o ./Core/Src/I2C.su ./Core/Src/Main.cyclo ./Core/Src/Main.d ./Core/Src/Main.o ./Core/Src/Main.su ./Core/Src/TIMERS.cyclo ./Core/Src/TIMERS.d ./Core/Src/TIMERS.o ./Core/Src/TIMERS.su ./Core/Src/UART.cyclo ./Core/Src/UART.d ./Core/Src/UART.o ./Core/Src/UART.su ./Core/Src/blue_unit_conversion.d ./Core/Src/blue_unit_conversion.o ./Core/Src/mainunused.cyclo ./Core/Src/mainunused.d ./Core/Src/mainunused.o ./Core/Src/mainunused.su ./Core/Src/stm32wb0x_hal_msp.cyclo ./Core/Src/stm32wb0x_hal_msp.d ./Core/Src/stm32wb0x_hal_msp.o ./Core/Src/stm32wb0x_hal_msp.su ./Core/Src/stm32wb0x_it.cyclo ./Core/Src/stm32wb0x_it.d ./Core/Src/stm32wb0x_it.o ./Core/Src/stm32wb0x_it.su ./Core/Src/syscalls.cyclo ./Core/Src/syscalls.d ./Core/Src/syscalls.o ./Core/Src/syscalls.su ./Core/Src/sysmem.cyclo ./Core/Src/sysmem.d ./Core/Src/sysmem.o ./Core/Src/sysmem.su ./Core/Src/system_stm32wb0x.cyclo ./Core/Src/system_stm32wb0x.d ./Core/Src/system_stm32wb0x.o ./Core/Src/system_stm32wb0x.su

.PHONY: clean-Core-2f-Src

