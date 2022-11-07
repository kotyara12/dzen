#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Повышаем уровень детализации ТОЛЬКО для данного файла до максимального
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

#define STACK_SIZE 4096

void logs_exec(void *pvParameters)
{
  while(1) {
    ESP_LOGE("TASK", "Error %d", 500);
    vTaskDelay(pdMS_TO_TICKS(1000));

    ESP_LOGI("TASK", "Info");
    vTaskDelay(pdMS_TO_TICKS(1000));
  };
  vTaskDelete(NULL);
}

void app_main() 
{
  // Программное изменение уровеня детализации журнала
  esp_log_level_set("*", ESP_LOG_ERROR);        // Для всех модулей: только ошибки
  esp_log_level_set("wifi", ESP_LOG_WARN);      // Только для WiFi: ошибки и предупреждения
  esp_log_level_set("dhcpc", ESP_LOG_INFO);     // Только для DHCP: ошибки, предупреждения и информация

  // Запускаем тестовую задачу
  static StaticTask_t xTaskBuffer;
  static StackType_t xStack[STACK_SIZE];
  TaskHandle_t xHandle = xTaskCreateStaticPinnedToCore(logs_exec, "logs", STACK_SIZE, NULL, 5, xStack, &xTaskBuffer, 1);
  if (xHandle == NULL) {
    ESP_LOGE("TASK", "Failed to task create");
  };
}
