#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_log.h"

TimerHandle_t _timer = NULL;

// Функция обратного вызова таймера
void timer_callback(TimerHandle_t pxTimer)
{
  // Это не обработчик прерываний, поэтому можно использовать ESP_LOGx
  ESP_LOGW("timer", "Software FreeRTOS timer alarm!");
}

void app_main() 
{
  // Создаем таймер
  _timer = xTimerCreate("Timer", // Просто текстовое имя для отладки
      pdMS_TO_TICKS(1000),       // Период таймера в тиках
      pdTRUE,                    // Повторяющийся таймер
      NULL,                      // Идентификатор, присваиваемый создаваемому таймеру
      timer_callback             // Функция обратного вызова
  );

  // Запускаем таймер
  if (xTimerStart( _timer, 0) == pdPASS) {
    ESP_LOGI("main", "Software FreeRTOS timer stated");
  };

  // Основной цикл
  while (1) {
    // Просто выводим сообщение в лог через каждые 5 секунд
    vTaskDelay(pdMS_TO_TICKS(5000));
    ESP_LOGI("main", "vTaskDelay(5000) timeout");
  };
}