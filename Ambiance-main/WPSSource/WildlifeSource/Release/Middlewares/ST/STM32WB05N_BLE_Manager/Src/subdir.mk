################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Middlewares/ST/STM32WB05N_BLE_Manager/Src/BLE_Manager.c 

OBJS += \
./Middlewares/ST/STM32WB05N_BLE_Manager/Src/BLE_Manager.o 

C_DEPS += \
./Middlewares/ST/STM32WB05N_BLE_Manager/Src/BLE_Manager.d 


# Each subdirectory must supply rules for building sources it contributes
Middlewares/ST/STM32WB05N_BLE_Manager/Src/%.o Middlewares/ST/STM32WB05N_BLE_Manager/Src/%.su Middlewares/ST/STM32WB05N_BLE_Manager/Src/%.cyclo: ../Middlewares/ST/STM32WB05N_BLE_Manager/Src/%.c Middlewares/ST/STM32WB05N_BLE_Manager/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0plus -std=gnu11 -DUSE_HAL_DRIVER -DSTM32WB05 -DUSE_NUCLEO_64 -c -I../Core/Inc -I../Drivers/STM32WB0x_HAL_Driver/Inc -I../Drivers/STM32WB0x_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32WB0X/Include -I../Drivers/CMSIS/Include -I"C:/Users/stipi/Desktop/HWstuff/ECE129/WildlifeSource/WPSSource/WildlifeSource/Drivers/BSP/STM32WB0x-nucleo" -I../Drivers/BSP/STM32WB0x-nucleo -I../Middlewares/ST/STM32WB05N_BLE_Manager/Inc -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Middlewares-2f-ST-2f-STM32WB05N_BLE_Manager-2f-Src

clean-Middlewares-2f-ST-2f-STM32WB05N_BLE_Manager-2f-Src:
	-$(RM) ./Middlewares/ST/STM32WB05N_BLE_Manager/Src/BLE_Manager.cyclo ./Middlewares/ST/STM32WB05N_BLE_Manager/Src/BLE_Manager.d ./Middlewares/ST/STM32WB05N_BLE_Manager/Src/BLE_Manager.o ./Middlewares/ST/STM32WB05N_BLE_Manager/Src/BLE_Manager.su

.PHONY: clean-Middlewares-2f-ST-2f-STM32WB05N_BLE_Manager-2f-Src

