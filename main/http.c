#include "http.h"
#include "view.h"
#include "config.h"

#include "cJSON.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

static const char *TAG = "HTTP_CLIENT";

static QueueHandle_t view_queue;

static uint16_t err_counter = 0;

static view_data_t http_recover = {
    .text = "",
    .type = RECOVER,
};
static view_data_t http_error = {
    .text = "HTTP CLIENT ERR!",
    .type = ERROR,
};

static void parse_json(char *str)
{
    cJSON *json = cJSON_Parse(str);

    if (json == NULL) {
        return;
    }

    cJSON *phenomena_json = cJSON_GetObjectItem(json, "phenomena");
    if (!cJSON_IsString(phenomena_json) || (phenomena_json->valuestring == NULL)) {
        cJSON_Delete(json);
        return;
    }

    cJSON *main_json = cJSON_GetObjectItem(json, "main");
    if (!cJSON_IsString(main_json) || (main_json->valuestring == NULL)) {
        cJSON_Delete(json);
        return;
    }

    view_data_t phenomena = {
        .text = phenomena_json->valuestring,
        .type = PHENOMENA,
    };

    view_data_t main = {
        .text = main_json->valuestring,
        .type = MAIN,
    };

    xQueueSend(view_queue, &phenomena, portMAX_DELAY);
    xQueueSend(view_queue, &main, portMAX_DELAY);

    if (err_counter > 0) {
        xQueueSend(view_queue, &http_recover, portMAX_DELAY);
        err_counter = 0;
    }

    /* FIX ME:
    *  giving some time to display information before data will gone:
    *  stack pointers, cJSON_Delete()... */
    vTaskDelay( 2000 / portTICK_PERIOD_MS);

    cJSON_Delete(json);
}

static esp_err_t http_event_handler(esp_http_client_event_t *http_event)
{
    switch (http_event->event_id) {
        case HTTP_EVENT_ERROR:
            //ESP_LOGI(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            //ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_ON_DATA:
            if (!esp_http_client_is_chunked_response(http_event->client)) {
                //ESP_LOGI(TAG, "%.*s", http_event->data_len, (char*)http_event->data);
                parse_json((char*)http_event->data);
            }
            break;
        case HTTP_EVENT_DISCONNECTED:
            //ESP_LOGI(TAG, "HTTP_EVENT_ON_DISCONNECTED");
            break;
        default:
            break;
    }
    return ESP_OK;
}

void http_xHandler(void *queue)
{
    view_queue = *(QueueHandle_t*)queue;

    esp_http_client_config_t config = {
        .url = CONFIG_API_URL,
        .event_handler = http_event_handler,
        .user_agent = CONFIG_API_USERAGENT,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    if (err != ESP_OK) {
        if (err_counter == CONFIG_API_SKIP_ERRORS) {
            xQueueSend(view_queue, &http_error, portMAX_DELAY);
        }
        //Overflow protection
        if (++err_counter == 0) { err_counter = CONFIG_API_SKIP_ERRORS + 1; }
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    vTaskDelete(NULL);
}
