#pragma once
#include "esphome/core/hal.h"
inline void (*icon_task)(void *)=nullptr;
inline void *icon_task_arg=nullptr;
inline int xTaskCreate(void (*task)(void *),const char *,int,void *arg,int,void *){icon_task=task;icon_task_arg=arg;return pdPASS;}
inline void vTaskDelete(void *){}
inline void vTaskDelay(unsigned){++icon_clock;}
inline void finish_icon_task(){auto task=icon_task;auto arg=icon_task_arg;icon_task=nullptr;icon_task_arg=nullptr;task(arg);}
