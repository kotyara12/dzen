#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

// Функция задачи 1
void task1_exec(void *pvParameters)
{
  // Организуем бесконечный цикл
  while(1) {
    // Выводим сообщение в терминал
    ESP_LOGI("TASK1", "Task 1 executed");
    // Пауза 10 секунд
    vTaskDelay(pdMS_TO_TICKS(100000));
  };
  // Сюда мы не должны добраться никогда. Но если "что-то пошло не так" - нужно всё-таки удалить задачу из памяти
  vTaskDelete(NULL);
}

void app_main() 
{

}