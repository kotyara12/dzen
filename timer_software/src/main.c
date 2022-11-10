#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_log.h"

// Функция обратного вызова таймера
static void timer_callback(void *args)
{
  // Это не обработчик прерываний, поэтому можно использовать ESP_LOGx
  ESP_LOGW("timer", "Software timer alarm!");
}

void app_main() 
{
  static esp_timer_handle_t _timer = NULL;

  // Настраиваем параметры таймера
  esp_timer_create_args_t config = {
    .name = "test_timer",              // Условное название таймера, ни на что не влияет
    .callback = timer_callback,        // Указатель на функцию обратного вызова для таймера
    .arg = NULL,                       // Какие-либо аргументы, которые можно передать в callback
    .dispatch_method = ESP_TIMER_TASK, // Обработчик будет вызыван из задачи (другой вариант - из ISR)
    .skip_unhandled_events = false     // Если какое-либо срабатывание таймера было пропущено, то обработать его в любом случае
  }; 

  // Создаем таймер
  esp_timer_create(&config, &_timer);
  if (_timer == NULL) {
    ESP_LOGE("main", "Failed to timer create");
    return;
  };

  // Запускаем таймер в периодическом режиме с интервалом 3 секунды
  esp_timer_start_periodic(_timer, 3*1000*1000);
  ESP_LOGI("main", "Software timer stated");

  // Основной цикл
  while (1) {
    // Просто выводим сообщение в лог через каждые 5 секунд
    vTaskDelay(pdMS_TO_TICKS(5000));
    ESP_LOGI("main", "vTaskDelay(5000) timeout");
  };
}