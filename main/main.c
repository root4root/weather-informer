#include "main.h"

//static const char *TAG = "MAIN";

//Main hardware initialization
static void main_init(void)
{
    /*--- REBOOT INDICATOR
     * Time to shutdown LED, that indicate uncontrolled reboot
     * LED ON - reboot has been occurred (some error took place)

    vTaskDelay( 2000 / portTICK_PERIOD_MS);

    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_GPIO, 1);

    gpio_reset_pin(BUTTON_GPIO);
    gpio_set_direction(BUTTON_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON_GPIO, GPIO_PULLUP_ONLY);

    //Hold button within 2s after reboot
    if (!gpio_get_level(BUTTON_GPIO)) {
        gpio_set_level(LED_GPIO, 0);
    } //--- REBOOT INDICATOR */

    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);

    i2c_master_bus_config_t i2c_bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = PORT_NUMBER,
        .scl_io_num = SCL_IO_PIN,
        .sda_io_num = SDA_IO_PIN,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config, &bus_handle));
}

void app_main(void)
{
    QueueHandle_t view_queue = xQueueCreate(5, sizeof(view_data_t));

    main_init();

    xTaskCreate(
        view_xHandler,
        "ViewTask",
        3072,
        (void*) &view_queue,
        5,
        NULL
    );

    wifi_init_sta(view_queue);

    xTaskCreate(
        rtc_xHandler,
        "RtcTask",
        3072, //2048 not enough, uxTaskGetStackHighWaterMark()
        (void*) &view_queue,
        5,
        NULL
    );

    uint16_t api_resync_timer = 0;

    while (1) {

        if (api_resync_timer == 0) {
            xTaskCreate(
                http_xHandler,
                "HttpTask",
                8192,
                (void*) &view_queue,
                5,
                NULL
            );
        }

        ++api_resync_timer;
        api_resync_timer %= CONFIG_API_UPD_PERIOD;

        vTaskDelay( 1000 / portTICK_PERIOD_MS);
    }
}
