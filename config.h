#pragma once

#define POINTING_DEVICE_ENABLE

// Активация аппаратного I2C1 на RP2040
#define RP2040_I2C_USE_I2C1 TRUE
#define I2C_DRIVER_I2CV1

// Стандартные пины I2C1 для Raspberry Pi Pico
#define I2C1_SDA_PIN GP2 // Физический пин 4
#define I2C1_SCL_PIN GP3 // Физический пин 5

// Скорость шины (Fast Mode)
#define I2C1_CLOCK_SPEED 400000 

// Параметры PixArt PCT1336QN
#define PCT1336_I2C_ADDRESS (0x5C << 1) // 7-битный адрес, сдвинутый для QMK API
#define PCT1336_TOUCH_REG 0x00
