#include "view.h"
#include "lcd.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
//#include "esp_log.h"

static void view_init(void)
{
    lcd_begin(0x27, 16, 2);

    lcd_clear();
    lcd_backlight();
}

void view_xHandler(void *queue)
{
    QueueHandle_t q = *(QueueHandle_t*)queue;
    view_data_t v_data;

    view_init();

    while (1) {
        xQueueReceive(q, &v_data, portMAX_DELAY);

        switch (v_data.type) {
            case PHENOMENA:
                lcd_setCursor(0, 0);
                lcd_writeString(v_data.text);
                break;
            case TIME:
                lcd_setCursor(11, 0);
                lcd_writeString(v_data.text);
                //ESP_LOGW("VIEW Stack: higher - better: ", "%d", uxTaskGetStackHighWaterMark(NULL));
                break;
            case MAIN:
                lcd_setCursor(0, 1);
                lcd_writeString(v_data.text);
                break;
            case ERROR:
                lcd_setCursor(0, 0);
                lcd_writeString("==========");
                lcd_setCursor(0, 1);
                lcd_writeString(v_data.text);
                lcd_noBacklight();
                break;
            case RECOVER:
                lcd_setCursor(0, 1);
                lcd_writeString(v_data.text);
                lcd_backlight();
                break;
            default:
                break;
        }
    }
}
