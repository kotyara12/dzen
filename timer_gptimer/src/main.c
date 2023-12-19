#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gptimer.h"
#include "esp_log.h"

// Функция обратного вызова таймера
static bool IRAM_ATTR gptimer_alarm_callback(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_data)
{
  // В обработчиках нельзя использовать ESP_LOGx(), вместо этого следует использовать ESP_DRAM_LOGx()!!!
  ESP_DRAM_LOGW("timer0", "Hardware timer alarm!");

  // Если вы вызывали функции FreeRTOS (например `xQueueSendFromISR`) из обработчика прерываний, 
  // вам необходимо вернуть значение true или false на основе возвращаемого значения аргумента pxHigherPriorityTaskWoken. 
  // Если возвращаемое значение `pxHigherPriorityTaskWoken` любых вызовов FreeRTOS равно pdTRUE, то вы должны вернуть true; в противном случае вернуть false.
  // ---
  // В данном простейшем случае мы не отправляли ничего в очереди, поэтому можно вернуть false
  return false;
}

void app_main() 
{
  gptimer_handle_t gptimer = NULL;

  // Настраиваем параметры таймера
  gptimer_config_t timer_config = {
      .clk_src = GPTIMER_CLK_SRC_DEFAULT,            // Выбираем источник тактового сигнала для счетчиков
      .direction = GPTIMER_COUNT_UP,                 // Устанавливаем направление счета
      .resolution_hz = 1000000, // 1MHz, 1 tick=1us  // Устанавливаем частоту счета, то есть минимальный интервал времени на 1 тик
  };
  
  // Создаем дексриптор GP-таймера с указанными параметрами
  gptimer_new_timer(&timer_config, &gptimer);

  // Подключаем функцию обратного вызова
  gptimer_event_callbacks_t cb_config = {
      .on_alarm = gptimer_alarm_callback,
  };
  gptimer_register_event_callbacks(gptimer, &cb_config, NULL);

  // Задаем начальное значение счетчика таймера (не обязательно)
  gptimer_set_raw_count(gptimer, 0);

  // Задаем параметры счетчика таймера
  gptimer_alarm_config_t alarm_config = {
      .alarm_count = 3000000,                   // Конечное значение счетчика = 3 секунды
      .reload_count = 0,                        // Значение счетчика при автосбросе
      .flags.auto_reload_on_alarm = true,       // Автоперезапуск счетчика таймера разрешен
  };
  gptimer_set_alarm_action(gptimer, &alarm_config);

  // Разрешаем прерывания для данного таймера
  gptimer_enable(gptimer);

  // Запускаем таймер
  gptimer_start(gptimer);
  ESP_LOGI("main", "Hardware timer stated");

  // Основной цикл
  while (1) {
    // Просто выводим сообщение в лог через каждые 5 секунд
    vTaskDelay(pdMS_TO_TICKS(5000));
    ESP_LOGI("main", "vTaskDelay(5000) timeout");
  }
}