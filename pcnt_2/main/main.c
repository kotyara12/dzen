#include "driver/pulse_cnt.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#define PCNT_INPUT_IO 4          // GPIO, к которому подключён источник импульсов 
#define PCNT_GLITCH_MAX 1000     // Длительность помехи в нс
#define PCNT_H_LIM_VAL 32767     // Максимальное значение счетчика 
#define PCNT_L_LIM_VAL -1        // Минимальное значение счетчика 
#define TIMER_TIMEOUT_US 1000000 // Таймаут обнаружения конца пачки, например 1 секунда (больше == надежнее, но задержка может мешать)

// Таймер обнаружения конца пачки
static esp_timer_handle_t batch_timer;

// Какая-то очередь, в которую мы будем скидывать результаты, она не обязательно должна быть объявлена здесь
static QueueHandle_t pcnt_evt_queue;

// Обработчик события PCNT watch point
static bool pcnt_on_reach(pcnt_unit_handle_t unit, const pcnt_watch_event_data_t *edata, void *user_ctx)
{
    // Перезапустим таймер пачки
    esp_timer_stop(batch_timer);
    esp_timer_start_once(batch_timer, TIMER_TIMEOUT_US);
    return false;
}

// Обработчик таймера окончания пачки
static void batch_timer_callback(void* arg)
{
    // Считываем значение счетчика
    int pulse_count = 0;
    pcnt_unit_get_count((pcnt_unit_handle_t)arg, &pulse_count);
    // Сбрасываем счётчик
    pcnt_unit_clear_count((pcnt_unit_handle_t)arg);
    // Отправить результат, например в очередь
    xQueueSend(pcnt_evt_queue, &pulse_count, 0);
}

void app_main(void)
{
    // Создадим очередь для передачи результата (это может быть где-то в другой задаче)
    pcnt_evt_queue = xQueueCreate(10, sizeof(int));
    
    // Шаг 1. Создаём PCNT unit
    pcnt_unit_config_t unit_config = {0};
    unit_config.high_limit = PCNT_H_LIM_VAL;        // Максимальное значение счетчика
    unit_config.low_limit = PCNT_L_LIM_VAL;         // Минимальное значение счетчика
    pcnt_unit_handle_t pcnt_unit = NULL;
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &pcnt_unit));

    // Шаг 2 (опционально). Включаем фильтр коротких помех в канале связи
    pcnt_glitch_filter_config_t filter_config = {0};
    filter_config.max_glitch_ns = PCNT_GLITCH_MAX;
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(pcnt_unit, &filter_config));

    // Шаг 3. Создаём PCNT channel и настроим счёт по фронтам импульса
    pcnt_chan_config_t channel_config = {0};
    channel_config.edge_gpio_num = PCNT_INPUT_IO;   // GPIO, к которому подключён источник импульсов
    channel_config.level_gpio_num = -1;             // Не используется
    pcnt_channel_handle_t pcnt_channel = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &channel_config, &pcnt_channel));
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_channel, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_HOLD));

    // Шаг 4. Настраиваем watch point и активируем его
    ESP_ERROR_CHECK(pcnt_unit_add_watch_point(pcnt_unit, 1));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));

    // Шаг 5. Зарегистрируем callback на созданный watch point
    pcnt_event_callbacks_t cbs = {0};
    cbs.on_reach = pcnt_on_reach;
    ESP_ERROR_CHECK(pcnt_unit_register_event_callbacks(pcnt_unit, &cbs, NULL));

    // Шаг 6. Создадим таймер для отслеживания окончания пачки
    esp_timer_create_args_t timer_args = {0};
    timer_args.callback = batch_timer_callback;
    timer_args.arg = (void*)pcnt_unit;
    timer_args.name = "batch_timer";
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &batch_timer));

    // Шаг 7. Включаем unit и запускаем счётчик
    ESP_ERROR_CHECK(pcnt_unit_enable(pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit));

    // Ожидание результата (это может быть где-то в другой задаче)
    int result = 0;
    while (1) {
        // Здесь result — количество импульсов в пачке
        if (xQueueReceive(pcnt_evt_queue, &result, portMAX_DELAY)) {
            ESP_LOGI("PCNT", "Recieved %d pulses", result);
        }
    }
}