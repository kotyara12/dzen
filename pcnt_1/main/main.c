#include "driver/pulse_cnt.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define PCNT_INPUT_IO 4        // GPIO, к которому подключён датчик 
#define PCNT_INPUT_COEF 2.25   // Коэффициент пересчета 2.25 мл на 1 импульс
#define PCNT_GLITCH_MAX 1000   // Длительность помехи в нс
#define PCNT_H_LIM_VAL 32767   // Максимальное значение счетчика 
#define PCNT_L_LIM_VAL -1      // Минимальное значение счетчика 

void app_main(void)
{
    // Шаг 1. Создаём PCNT unit
    pcnt_unit_config_t unit_config = {0};
    unit_config.low_limit = PCNT_L_LIM_VAL;         // Минимальное значение счетчика
    unit_config.high_limit = PCNT_H_LIM_VAL;        // Максимальное значение счетчика

    pcnt_unit_handle_t pcnt_unit = NULL;
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &pcnt_unit));

    // Шаг 2. Создаём PCNT channel
    pcnt_chan_config_t channel_config = {0};
    channel_config.edge_gpio_num = PCNT_INPUT_IO;   // GPIO, к которому подключён датчик 
    channel_config.level_gpio_num = -1;             // Не используется

    pcnt_channel_handle_t pcnt_channel = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &channel_config, &pcnt_channel));

    // Шаг 3 (опционально). Настраиваем встроенную подтяжку GPIO к земле (или питанию - выберите нужное)
    ESP_ERROR_CHECK(gpio_set_pull_mode((gpio_num_t)PCNT_INPUT_IO, GPIO_PULLDOWN_ONLY));

    // Шаг 4 (опционально). Включаем фильтр debounce
    pcnt_glitch_filter_config_t filter_config = {0};
    filter_config.max_glitch_ns = PCNT_GLITCH_MAX;
    
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(pcnt_unit, &filter_config));

    // Шаг 5. Включаем unit и запускаем счёт
    ESP_ERROR_CHECK(pcnt_unit_enable(pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit));

    while (1) {
        int count = 0; 
        // Считываем значение счетчика
        ESP_ERROR_CHECK(pcnt_unit_get_count(pcnt_unit, &count));
        // Сбросить счётчик для следующего измерения
        ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));

        // Пересчитываем в скорость потока
        float flow_ml_per_sec = (float)count * PCNT_INPUT_COEF; 
        ESP_LOGI("FLOW", "Flow: %d pulses/sec, %.2f ml/sec", count, flow_ml_per_sec); 

        // Ждем заданный интервал времени
        vTaskDelay(pdMS_TO_TICKS(1000)); // 1 секунда
    }
}