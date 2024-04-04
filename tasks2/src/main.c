#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_random.h"
#include "bootloader_random.h"

#define DEMO_VARIANT 6

#if DEMO_VARIANT == 1
  // Простой автономный цикл измерений с постоянным интервалом
  #include "task01.h"     
#elif DEMO_VARIANT == 2
  // Простой автономный цикл измерений с адаптивным интервалом
  #include "task02.h"
#elif DEMO_VARIANT == 3
  // Задача, ожидающая входящих данных из входящей очереди
  #include "task03.h"
#elif DEMO_VARIANT == 4
  // Задача с передачей строк переменной длины посредством очереди
  #include "task04.h"
  #include "dyn_strings.h"
#elif DEMO_VARIANT == 5
  // Цикл измерений с постоянным интервалом и реакцией на внешние события
  #include "task05.h"
#elif DEMO_VARIANT == 6
  // Цикл измерений с адаптивным интервалом и реакцией на внешние события
  #include "task06.h"
#endif


void app_main() 
{
  // Инициализация генератора RNG, если RF (WiFi/BT) отключены
  bootloader_random_enable();

  // Запуск прикладной задачи из локальной библиотеки
  app_task_start();

  #if DEMO_VARIANT == 3
    // Бесконечный цикл передачи данных в очередь задачи #03
    while (1) {
      insertValueIntoQueue((float)esp_random() / 100000000.0);
      vTaskDelay(pdMS_TO_TICKS(10000));
    };
  #elif DEMO_VARIANT == 4
    // Бесконечный цикл передачи строк в очередь задачи #04
    uint32_t i = 0;
    while (1) {
      i++;
      char* text = malloc_stringf("String value: %i", i);
      insertStringIntoQueue(text);
      vTaskDelay(pdMS_TO_TICKS(10000));
    };
  #endif
}