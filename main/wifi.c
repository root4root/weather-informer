#include "wifi.h"
#include "config.h"
#include "view.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#define WEATHER_ESP_WIFI_SSID      CONFIG_WIFI_SSID
#define WEATHER_ESP_WIFI_PASS      CONFIG_WIFI_PASSWORD
#define WEATHER_ESP_MAXIMUM_RETRY  CONFIG_WIFI_MAXRETRY

#if CONFIG_ESP_WIFI_AUTH_OPEN
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WEP
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *TAG = "Wi-Fi_STATION";

static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;

static QueueHandle_t view_queue;

static view_data_t wifi_error = {
    .text = "WI-FI CONN ERR!!",
    .type = ERROR,
};
static view_data_t wifi_recover = {
    .text = "WI-FI CONNECTED!",
    .type = RECOVER,
};

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
        return;
    }

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        xQueueSend(view_queue, &wifi_error, portMAX_DELAY);

        if (s_retry_num++ < WEATHER_ESP_MAXIMUM_RETRY) {
            vTaskDelay(30000 / portTICK_PERIOD_MS);
            esp_wifi_connect();
            ESP_LOGW(TAG, "Retry to connect to the AP %d", s_retry_num);
            return;
        }

        xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);

        ESP_LOGE(TAG, "Connect to the AP fail");
        return;
    }

    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        s_retry_num = 0;
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);

        xQueueSend(view_queue, &wifi_recover, portMAX_DELAY);
        ESP_LOGI(TAG, "Got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    }
}

void wifi_init_sta(QueueHandle_t queue)
{
    view_queue = queue;

    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        &event_handler,
        NULL,
        &instance_any_id));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        &event_handler,
        NULL,
        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WEATHER_ESP_WIFI_SSID,
            .password = WEATHER_ESP_WIFI_PASS,
            .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
//  ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    EventBits_t bits = xEventGroupWaitBits(
            s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to AP SSID: %s", WEATHER_ESP_WIFI_SSID);
        return;
    }

    if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed connect to AP SSID: %s", WEATHER_ESP_WIFI_SSID);
        return;
    }

    ESP_LOGE(TAG, "UNEXPECTED EVENT");

}
