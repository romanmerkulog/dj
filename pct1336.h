#pragma once
#include <stdint.h>
#include "pointing_device.h"

#define MAX_TOUCHES 3 // Нам нужно максимум 3 пальца для реализации ТЗ

typedef struct {
    uint16_t x;
    uint16_t y;
    bool active;
} touch_point_t;

void pct1336_init(void);
void pct1336_read(report_mouse_t* mouse_report);
