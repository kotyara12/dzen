#include <stddef.h>
#include "esp_log.h"
#include "esp_bit_defs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#define FLAG_0 (1<<0)
#define FLAG_1 (1<<1)
#define FLAG_2 (1<<2)

#define STACK_SIZE 4096

// Глобальный (статический) буфер под группу событий
StaticEventGroup_t xEventBuffer;
// Указатель на группу событий
EventGroupHandle_t xEventGroup = NULL;

// Буфер под служебные данные задачи
static StaticTask_t xTaskBuffer;
// Буфер под стек задачи
static StackType_t xStack[STACK_SIZE];

// Функция задачи 1
void task1_exec(void *pvParameters)
{
  EventBits_t uxBits;
  while(1) {
    uxBits = xEventGroupWaitBits(
      xEventGroup,               // Указатель на группу событий
      FLAG_0 | FLAG_1 | FLAG_2,  // Какие биты мы ждем
      pdTRUE,                    // Сбросить утановленные биты, после того как они были прочитаны
      pdFALSE,                   // Любой бит (даже один) приведет к выходу из ожидания
      pdMS_TO_TICKS(10000));     // Период ожидания 10 секунд

    // Проверяем, какие биты были установлены
    if ((uxBits & (FLAG_0 | FLAG_1 | FLAG_2)) != 0) {
      // Какой-то бит или несколько был установлен(ы), осталось выяснить какой
      if ((uxBits & FLAG_0) != 0) ESP_LOGI("EventGroups", "FLAG0 is set");
      if ((uxBits & FLAG_1) != 0) ESP_LOGI("EventGroups", "FLAG1 is set");
      if ((uxBits & FLAG_2) != 0) ESP_LOGI("EventGroups", "FLAG2 is set");
    } else {
      // Ни один бит не был установлен
      ESP_LOGI("EventGroups", "Timeout expired");
    };
  };
  vTaskDelete(NULL);
}

void app_main() 
{
  // Создание группы статическим методом
  xEventGroup = xEventGroupCreateStatic(&xEventBuffer);
  // Очищаем все рабочие биты группы
  xEventGroupClearBits(xEventGroup, 0x00FFFFFFU);

  // Запуск задачи статическим методом
  xTaskCreateStaticPinnedToCore(task1_exec, "task1", STACK_SIZE, NULL, 5, xStack, &xTaskBuffer, 1);

  // Демонстрационный цикл
  while (1) {
    // Ждем 3 секунды и устанавливаем бит FLAG_1
    vTaskDelay(pdMS_TO_TICKS(3000));
    xEventGroupSetBits(xEventGroup, FLAG_1);

    // Ждем 2 секунды и устанавливаем бит FLAG_0
    vTaskDelay(pdMS_TO_TICKS(2000));
    xEventGroupSetBits(xEventGroup, FLAG_0);

    // Ждем 4 секунды и устанавливаем биты FLAG_0 и FLAG_2 одновременно
    vTaskDelay(pdMS_TO_TICKS(4000));
    xEventGroupSetBits(xEventGroup, FLAG_0 | FLAG_2);

    // Ждем 20 секунд (для таймамута) и устанавливаем все биты одновременно
    vTaskDelay(pdMS_TO_TICKS(20000));
    xEventGroupSetBits(xEventGroup, FLAG_0 | FLAG_1 | FLAG_2);
  };
}