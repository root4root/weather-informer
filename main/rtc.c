#include "rtc.h"
#include "esp_sntp.h"
#include "view.h"
#include "config.h"

#include <string.h>
#include <time.h>
#include "freertos/idf_additions.h"
#include "esp_netif_sntp.h"
#include "esp_log.h"

#define LOOP_PERIOD_MS 500
#define SNTP_SKIP_ERRORS 3

static const char *TAG = "SNTP_CLIENT";

static QueueHandle_t view_queue;

static char strftime_buf[10];

static view_data_t data = {
        .text = strftime_buf,
        .type = TIME,
};

static uint32_t sntp_check_timer = 1;
static uint32_t sntp_check_timer_reset_ltcks;

static uint8_t sntp_sync_errs = 0;

static void sntp_check_sync(void);

static void rtc_init(void)
{
    /* Set timer to twice longer than CONFIG_LWIP_SNTP_UPDATE_DELAY
     * in while(1) loop ticks (ltcks) */ 
    sntp_check_timer_reset_ltcks = CONFIG_LWIP_SNTP_UPDATE_DELAY / 500;
    sntp_check_timer_reset_ltcks *= 1000 / LOOP_PERIOD_MS;

    setenv("TZ", "UTC-3", 1);
    tzset();

    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG(CONFIG_NTP_SERVER);
    esp_netif_sntp_init(&config);

    if (esp_netif_sntp_sync_wait(pdMS_TO_TICKS(10000)) == ESP_OK) {
        return;
    }

    strcpy(strftime_buf, "I_ERR");
    xQueueSend(view_queue, &data, portMAX_DELAY);

    ESP_LOGE(TAG, "Failed to update system time within 10s timeout");

    sntp_sync_errs = SNTP_SKIP_ERRORS + 1;
}

void rtc_xHandler(void *queue)
{
    view_queue = *(QueueHandle_t*)queue;
    rtc_init();

    time_t now;
    struct tm timeinfo, timeinfo_old;

    time(&now);
    localtime_r(&now, &timeinfo_old);
    ++timeinfo_old.tm_min;

    while(1) {

        time(&now);
        localtime_r(&now, &timeinfo);

        if (sntp_check_timer == 0) {
            sntp_check_sync();
        }

        if (timeinfo_old.tm_min != timeinfo.tm_min && sntp_sync_errs <= SNTP_SKIP_ERRORS) {
            strftime(strftime_buf, sizeof(strftime_buf), "%H:%M", &timeinfo);
            xQueueSend(view_queue, &data, portMAX_DELAY);
            timeinfo_old = timeinfo;
            //ESP_LOGW(TAG, "Stack: higher - better: %d", uxTaskGetStackHighWaterMark(NULL));
        }

        ++sntp_check_timer;
        sntp_check_timer %= sntp_check_timer_reset_ltcks;

        vTaskDelay( LOOP_PERIOD_MS / portTICK_PERIOD_MS);
    }
}

static void sntp_check_sync(void)
{
    sntp_sync_status_t sync_status = sntp_get_sync_status();

    if (sync_status != SNTP_SYNC_STATUS_RESET) {
        sntp_sync_errs = 0;
        return;
    }

    if (sntp_sync_errs == SNTP_SKIP_ERRORS) {
        strcpy(strftime_buf, "S_ERR");
        xQueueSend(view_queue, &data, portMAX_DELAY);
    }

    //Overflow protection
    if (++sntp_sync_errs == 0) { sntp_sync_errs = SNTP_SKIP_ERRORS + 1; }

    ESP_LOGE(TAG, "Can't resync...");
}
