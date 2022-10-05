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

#define STACK_SIZE 4096

void app_main() 
{
  // Буфер под служебные данные задачи
  static StaticTask_t xTaskBuffer;
  // Буфер под стек задачи
  static StackType_t xStack[STACK_SIZE];

  // Запуск задачи с статическим выделением памяти
  TaskHandle_t xHandle = xTaskCreateStaticPinnedToCore(task1_exec, "task1", STACK_SIZE, NULL, 5, xStack, &xTaskBuffer, 1);
  // Проверим, создалась ли задача
  if (xHandle == NULL) {
    ESP_LOGE("TASK1", "Failed to task create");
  };
}