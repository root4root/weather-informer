#ifndef WIFI_H
#define WIFI_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

void wifi_init_sta(QueueHandle_t queue);

#endif //WIFI_H
