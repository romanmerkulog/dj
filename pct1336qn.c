#include "pct1336qn.h"
#include "pointing_device.h"
#include "hal.h"
#include "print.h"

#define PCT1339_ADDR        0x66 // 8-bit адрес (0x33 << 1)
#define REG_BANK_SWITCH     0x7F
#define BANK5_MOTION        0x05

#define INT_PORT            IOPORT1     // GP0 на Pi Pico
#define INT_PIN_NUM         0

static const I2CConfig i2c_cfg = {};

__attribute__((weak)) void keyboard_post_init_user(void) {}
__attribute__((weak)) report_mouse_t pointing_device_task_user(report_mouse_t mouse_report) { return mouse_report; }

// Сервисная функция записи регистра
static void log_write_reg(uint8_t reg, uint8_t val) {
    uint8_t tx_buf[] = {reg, val};
    msg_t msg = i2cMasterTransmitTimeout(&I2CD1, PCT1339_ADDR, tx_buf, 2, NULL, 0, TIME_MS2I(10));
    if (msg == MSG_OK) {
        uprintf("I2C LOG: Запись в Reg 0x%02X -> 0x%02X [УСПЕШНО]\n", reg, val);
    } else {
        uprintf("I2C LOG: Ошибка записи в Reg 0x%02X (Код ошибки ОС: %d)\n", reg, msg);
    }
}

void keyboard_post_init_kb(void) {
    // Настраиваем только прерывание
    palSetPadMode(INT_PORT, INT_PIN_NUM, PAL_MODE_INPUT_PULLUP);

    // Старт I2C
    i2cStart(&I2CD1, &i2c_cfg);
    chThdSleepMilliseconds(50);

    uprintf("\n--- СТАРТ СЕССИИ ОТЛАДКИ ТРЕКПАДА PIXART ---\n");

    // Пытаемся переключить банк памяти
    log_write_reg(REG_BANK_SWITCH, BANK5_MOTION);
    chThdSleepMilliseconds(10);

    keyboard_post_init_user();
}

report_mouse_t pointing_device_task_kb(report_mouse_t mouse_report) {
    static bool last_int_state = true;
    bool current_int_state = palReadPad(INT_PORT, INT_PIN_NUM);

    // Если состояние пина INT изменилось (палец прикоснулся или отпустил)
    if (current_int_state != last_int_state) {
        uprintf("INT CHANGED: Физический уровень пина INT упал в: %d\n", current_int_state);
        last_int_state = current_int_state;
    }

    // Если тачпад держит прерывание (активный низкий уровень)
    if (current_int_state == PAL_LOW) {
        uint8_t rx_buf[7] = {0};
        uint8_t start_reg = 0x00;
        uint8_t tx_buf[] = {start_reg};

        // Прямая транзакция чтения
        msg_t msg = i2cMasterTransmitTimeout(&I2CD1, PCT1339_ADDR, tx_buf, 1, rx_buf, 7, TIME_MS2I(20));

        if (msg == MSG_OK) {
            // Выводим в логгер честный дамп памяти, полученный от тачпада
            uprintf("DATA DUMP [0x00-0x06]: %02X %02X %02X %02X %02X %02X %02X\n",
                    rx_buf[0], rx_buf[1], rx_buf[2], rx_buf[3], rx_buf[4], rx_buf[5], rx_buf[6]);
        } else {
            // Если транзакция сорвалась (например, чип выдал NACK)
            uprintf("I2C READ ERROR: Не удалось прочитать буфер. Код ОС ChibiOS: %d\n", msg);

            // Защитная микропауза, чтобы не забить логгер ошибками в дедлоке
            chThdSleepMilliseconds(100);
        }
    }

    // Возвращаем пустой репорт, чтобы мышь не сходила с ума во время тестов шины
    return pointing_device_task_user(mouse_report);
}
