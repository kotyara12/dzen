#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/pulse_cnt.h"
#include "driver/gpio.h"

static const char *TAG = "PCNT";

#define EXAMPLE_PCNT_HIGH_LIMIT 100
#define EXAMPLE_PCNT_LOW_LIMIT  -100

#define EXAMPLE_EC11_GPIO_A 4
#define EXAMPLE_EC11_GPIO_B 16

// Какая-то очередь, в которую мы будем скидывать результаты, она не обязательно должна быть объявлена здесь
static QueueHandle_t pcnt_evt_queue;

// Обработчик события PCNT watch point
static bool pcnt_on_reach(pcnt_unit_handle_t unit, const pcnt_watch_event_data_t *edata, void *user_ctx)
{
    // Считываем значение счетчика
    int pulse_count = 0;
    pcnt_unit_get_count(unit, &pulse_count);
    // Сбрасываем счётчик
    pcnt_unit_clear_count(unit);
    // Отправить результат, например в очередь
    xQueueSend(pcnt_evt_queue, &pulse_count, 0);
    return false;
}

void app_main(void)
{
    // Создадим очередь для передачи результата (это может быть где-то в другой задаче)
    pcnt_evt_queue = xQueueCreate(10, sizeof(int));

    // Шаг 1. Создаём PCNT unit
    ESP_LOGI(TAG, "install pcnt unit");
    pcnt_unit_config_t unit_config = {
        .high_limit = EXAMPLE_PCNT_HIGH_LIMIT,
        .low_limit = EXAMPLE_PCNT_LOW_LIMIT,
    };
    pcnt_unit_handle_t pcnt_unit = NULL;
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &pcnt_unit));

    // Шаг 2. Включаем фильтр debounce
    ESP_LOGI(TAG, "set glitch filter");
    pcnt_glitch_filter_config_t filter_config = {
        .max_glitch_ns = 1000,
    };
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(pcnt_unit, &filter_config));

    // Шаг 3. Создаём PCNT каналы
    ESP_LOGI(TAG, "install pcnt channels");

    // Шаг 3.1. Создаём PCNT канал А
    pcnt_chan_config_t chan_a_config = {
        .edge_gpio_num = EXAMPLE_EC11_GPIO_A,
        .level_gpio_num = EXAMPLE_EC11_GPIO_B,
    };
    pcnt_channel_handle_t pcnt_chan_a = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_a_config, &pcnt_chan_a));

    // Шаг 3.2. Создаём PCNT канал B
    pcnt_chan_config_t chan_b_config = {
        .edge_gpio_num = EXAMPLE_EC11_GPIO_B,
        .level_gpio_num = EXAMPLE_EC11_GPIO_A,
    };
    pcnt_channel_handle_t pcnt_chan_b = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_b_config, &pcnt_chan_b));

    // Шаг 4. Задаем реакции на импульсы в разных каналах
    ESP_LOGI(TAG, "set edge and level actions for pcnt channels");
    // ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_a, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE));
    // ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));
    // ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_b, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE));
    // ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_chan_b, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_a, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_HOLD));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_HOLD));
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_b, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_HOLD));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_chan_b, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_HOLD));

    // Шаг 5. Настраиваем watch point-ы
    ESP_ERROR_CHECK(pcnt_unit_add_watch_point(pcnt_unit, 1));
    ESP_ERROR_CHECK(pcnt_unit_add_watch_point(pcnt_unit, -1));

    // Шаг 6. Зарегистрируем callback на созданный watch point
    pcnt_event_callbacks_t cbs = {0};
    cbs.on_reach = pcnt_on_reach;
    ESP_ERROR_CHECK(pcnt_unit_register_event_callbacks(pcnt_unit, &cbs, NULL));

    // Шаг 7. Включаем и запускаем счетчик
    ESP_LOGI(TAG, "enable pcnt unit");
    ESP_ERROR_CHECK(pcnt_unit_enable(pcnt_unit));
    ESP_LOGI(TAG, "clear pcnt unit");
    ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));
    ESP_LOGI(TAG, "start pcnt unit");
    ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit));

    // Ожидание результата (это может быть где-то в другой задаче)
    int result = 0;
    while (1) {
        if (xQueueReceive(pcnt_evt_queue, &result, portMAX_DELAY)) {
            ESP_LOGI("PCNT", "Rotation: %d", result);
        }
    }
}
