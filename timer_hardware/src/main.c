#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/timer.h"
#include "esp_log.h"

// Обработчик прерываний таймера
static bool IRAM_ATTR timer_isr_callback(void *args)
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
  // Настраиваем параметры таймера
  timer_config_t config = {
    .divider = 80,                      // Задаем параметры предделителя (80 = минимальный интервал 1 микросекунда)
    .counter_dir = TIMER_COUNT_UP,      // Нормальное направление счета (вверх)
    .counter_en = TIMER_PAUSE,          // При создании таймера он будет поставлен на паузу
    .alarm_en = TIMER_ALARM_EN,         // Разрешить генерацию тревожного события (прерывания) по переполнению счетчика
    .auto_reload = TIMER_AUTORELOAD_EN, // Включить автоматическую перезагрузку: чип перезапустит счетчик после тревожного события
  }; // default clock source is APB
  timer_init(TIMER_GROUP_0, TIMER_0, &config);

  // Настраиваем НАЧАЛЬНОЕ значение счетчика
  // Счетчик таймера начнёт свой счет со значения, указанного ниже. 
  // Если установлено auto_reload, это значение будет также автоматически устанавливаться при тревоге
  timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0);

  // Настраиваем ПОРОГОВОЕ значение счетчика, при достижении которого будет сгенерировано событие тревоги
  // Значение задается в TIMER_BASE_CLK / TIMER_DIVIDER, то есть в нашем случае это: 80 MHz / 80 = 1MHz или 1 микросекунда
  // В нашаем случае это три секунды
  timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, 3*1000*1000);

  // Разрешаем прерывания для данного таймера
  timer_isr_callback_add(TIMER_GROUP_0, TIMER_0, timer_isr_callback, NULL, 0);
  timer_enable_intr(TIMER_GROUP_0, TIMER_0);

  // Запускаем таймер
  timer_start(TIMER_GROUP_0, TIMER_0);
  ESP_LOGI("main", "Hardware timer stated");

  // Основной цикл
  while (1) {
    // Просто выводим сообщение в лог через каждые 5 секунд
    vTaskDelay(pdMS_TO_TICKS(5000));
    ESP_LOGI("main", "vTaskDelay(5000) timeout");
  }
}