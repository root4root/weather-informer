#ifndef CONFIG_H
#define CONFIG_H

#define CONFIG_WIFI_SSID        "my_mysterious_SSID"
#define CONFIG_WIFI_PASSWORD    "my_very_secret_PASSWORD"
#define CONFIG_WIFI_MAXRETRY    120
//Each retry after 30s

#define CONFIG_SCL_IO_PIN       22
#define CONFIG_SDA_IO_PIN       21
#define CONFIG_MASTER_FREQUENCY 100000
#define CONFIG_PORT_NUMBER      I2C_NUM_0

#define CONFIG_NTP_SERVER       "10.10.10.10"

#define CONFIG_API_URL          "http://10.10.10.10:8000"
#define CONFIG_API_USERAGENT    "IoT Device 1.0"
//How often update information via API, in seconds
#define CONFIG_API_UPD_PERIOD   300
//How many API requests can fail (in a row) before an error is triggered
#define CONFIG_API_SKIP_ERRORS  1

#endif //CONFIG_H
