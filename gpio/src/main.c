#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define STACK_SIZE 4096

// Функция задачи
void led_exec(void *pvParameters)
{
  // Настраиваем вывод GPIO_NUM_12 на выход без подтяжки
  gpio_pad_select_gpio(12);
  gpio_set_direction(GPIO_NUM_12, GPIO_MODE_OUTPUT);
  gpio_set_pull_mode(GPIO_NUM_12, GPIO_FLOATING);

  // Мигание светодиодом
  while(1) {
    // Устанавливаем высокий уровень
    gpio_set_level(GPIO_NUM_12, 1);
    // Пауза 500 секунд
    vTaskDelay(pdMS_TO_TICKS(500));
    // Устанавливаем низкий уровень
    gpio_set_level(GPIO_NUM_12, 0);
    // Пауза 500 секунд
    vTaskDelay(pdMS_TO_TICKS(500));
  };
  vTaskDelete(NULL);
}

void app_main() 
{
  static StaticTask_t xTaskBuffer;
  static StackType_t xStack[STACK_SIZE];

  TaskHandle_t xHandle = xTaskCreateStaticPinnedToCore(led_exec, "led", STACK_SIZE, NULL, 5, xStack, &xTaskBuffer, 1);
  if (xHandle == NULL) {
    ESP_LOGE("TASK", "Failed to task create");
  };
}