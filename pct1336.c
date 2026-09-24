#include "pct1336.h"
#include "i2c_master.h"
#include "timer.h"
#include <stdlib.h>

#define TAP_TIMEOUT 180      // Максимальная длительность тапа (мс)
#define CLICK_HOLD_TIME 50   // Сколько времени удерживать клик для ОС (мс)
#define SCROLL_DEADZONE 8    // Порог скролла (подбирается после дебага)
#define MOUSE_DEADZONE 3     // Мертвая зона для курсора

static touch_point_t prev_points[MAX_TOUCHES];
static uint8_t prev_touch_count = 0;
static uint32_t touch_start_time = 0;
static bool is_gesture_drag = false;

// Таймеры для искусственного удержания кликов
static uint32_t left_click_timer = 0;
static uint32_t middle_click_timer = 0;
static bool left_click_active = false;
static bool middle_click_active = false;

void pct1336_init(void) {
    i2c_init(); 
    for (int i = 0; i < MAX_TOUCHES; i++) {
        prev_points[i].active = false;
    }
}

void pct1336_read(report_mouse_t* mouse_report) {
    uint8_t buffer[18]; // Буфер для 3 пальцев
    
    i2c_status_t status = i2c_readReg(PCT1336_I2C_ADDRESS, PCT1336_TOUCH_REG, buffer, sizeof(buffer), 100);
    if (status != I2C_STATUS_SUCCESS) {
        return; 
    }

    touch_point_t curr_points[MAX_TOUCHES] = {0};
    uint8_t curr_touch_count = 0;

    // Сбор координат
    for (int i = 0; i < MAX_TOUCHES; i++) {
        int offset = i * 6; 
        uint8_t touch_status = buffer[offset];
        
        if (touch_status & 0x80) { 
            curr_points[i].active = true;
            curr_points[i].x = (buffer[offset + 1] << 8) | buffer[offset + 2];
            curr_points[i].y = (buffer[offset + 3] << 8) | buffer[offset + 4];
            curr_touch_count++;
        }
    }

    // Фиксация начала касания
    if (curr_touch_count > 0 && prev_touch_count == 0) {
        touch_start_time = timer_read32();
        is_gesture_drag = false;
    }

    // Сброс осей перемещения
    mouse_report->x = 0;
    mouse_report->y = 0;
    mouse_report->v = 0;
    mouse_report->h = 0;

    // Алгоритм жестов
    if (curr_touch_count == 1 && prev_points[0].active && curr_points[0].active) {
        int16_t dx = (int16_t)curr_points[0].x - prev_points[0].x;
        int16_t dy = (int16_t)curr_points[0].y - prev_points[0].y;

        if (abs(dx) > MOUSE_DEADZONE || abs(dy) > MOUSE_DEADZONE) {
            mouse_report->x = dx / 4;  // Делитель подберите при дебаге
            mouse_report->y = -(dy / 4); // Инверсия Y
            is_gesture_drag = true; 
        }
    } 
    else if (curr_touch_count == 2 && prev_points[0].active && curr_points[0].active) {
        int16_t dy = (int16_t)curr_points[0].y - prev_points[0].y;
        int16_t dx = (int16_t)curr_points[0].x - prev_points[0].x;

        if (abs(dy) > SCROLL_DEADZONE) {
            mouse_report->v = (dy > 0) ? 1 : -1;
            is_gesture_drag = true;
        }
        if (abs(dx) > SCROLL_DEADZONE) {
            mouse_report->h = (dx > 0) ? 1 : -1;
            is_gesture_drag = true;
        }
    }
    else if (curr_touch_count == 3) {
        is_gesture_drag = true; // Запрет перемещения при 3 пальцах
    }

    // Обработка отпускания (триггер клика)
    if (curr_touch_count == 0 && prev_touch_count > 0) {
        uint32_t touch_duration = timer_elapsed32(touch_start_time);

        if (touch_duration < TAP_TIMEOUT && !is_gesture_drag) {
            if (prev_touch_count == 1) {
                left_click_active = true;
                left_click_timer = timer_read32();
            } 
            else if (prev_touch_count == 3) {
                middle_click_active = true;
                middle_click_timer = timer_read32();
            }
        }
    }

    // Поддержание кликов по таймеру
    if (left_click_active) {
        if (timer_elapsed32(left_click_timer) < CLICK_HOLD_TIME) {
            mouse_report->buttons |= MOUSE_BTN1;
        } else {
            left_click_active = false;
        }
    }

    if (middle_click_active) {
        if (timer_elapsed32(middle_click_timer) < CLICK_HOLD_TIME) {
            mouse_report->buttons |= MOUSE_BTN3;
        } else {
            middle_click_active = false;
        }
    }

    // Сохранение состояния
    for (int i = 0; i < MAX_TOUCHES; i++) {
        prev_points[i] = curr_points[i];
    }
    prev_touch_count = curr_touch_count;
}
