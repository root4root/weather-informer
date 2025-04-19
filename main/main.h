#include <time.h>
#include <stdint.h>

#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif_sntp.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"

#include "config.h"
#include "wifi.h"
#include "http.h"
#include "view.h"
#include "rtc.h"

#define SCL_IO_PIN CONFIG_SCL_IO_PIN
#define SDA_IO_PIN CONFIG_SDA_IO_PIN
#define MASTER_FREQUENCY CONFIG_MASTER_FREQUENCY
#define PORT_NUMBER CONFIG_PORT_NUMBER

// Builtin LED on D2
#define LED_GPIO     2
// D0, builtin BOOT button, pulled up
#define BUTTON_GPIO  0
