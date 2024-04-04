#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_random.h"
#include "esp_log.h"
#include "task02.h"

const char* logTAG  = "TASK2";

#define APP_TASK_STACK_SIZE 4096
#define APP_TASK_PRIORITY   5
#define APP_TASK_CORE       1

// Статический буфер под служебные данные задачи
static StaticTask_t xTaskBuffer;
// Статический буфер под стек задачи
static StackType_t xStack[APP_TASK_STACK_SIZE];

// Получение результатов измерений датчика
float readSensor1()
{
  // В данном демопроекте мы ничего мерять не будем - это функция-имитация.
  // Но вы можете вставить сюда реальное чтение датчика
  return (float)esp_random() / 100000000.0;
}

// Отправка результатов измерений куда-то на сервер
void sendHttpRequest(const float value)
{
  // В данном демопроекте мы ничего отправлять не будем - это функция-имитация.
  // Но вы можете вставить сюда реальную отправку данных на серсер HTTP-запросом
  uint32_t delay = esp_random() / 10000000;
  ESP_LOGI(logTAG, "Send data, delay %u ms", (uint)delay);
  vTaskDelay(pdMS_TO_TICKS(delay));
}

// Функция задачи
void app_task_exec(void *pvParameters)
{
  uint8_t i = 0;
  TickType_t prevWakeup = 0;

  // Бесконечный цикл задачи
  while(1) {
    // Читаем данные с сенсора
    float value = readSensor1();

    // Выводим сообщение в терминал
    ESP_LOGI(logTAG, "Read sensor: value = %.1f", value);

    // Один раз в 30 секунд отправляем данные на сервер
    i++;
    if (i >= 3) {
      i = 0;
      sendHttpRequest(value);
    };
    
    // Пауза 10 секунд
    vTaskDelayUntil(&prevWakeup, pdMS_TO_TICKS(10000));
  };

  // Сюда мы не должны добраться никогда. Но если "что-то пошло не так" - нужно всё-таки удалить задачу из памяти
  vTaskDelete(NULL);
}

void app_task_start()
{
  // Запуск задачи с статическим выделением памяти
  TaskHandle_t xHandle = xTaskCreateStaticPinnedToCore(app_task_exec, "app_task", 
    APP_TASK_STACK_SIZE, NULL, APP_TASK_PRIORITY, xStack, &xTaskBuffer, APP_TASK_CORE);
  if (xHandle == NULL) {
    ESP_LOGE(logTAG, "Failed to task create");
  } else {
    ESP_LOGI(logTAG, "Task started");
  };
}

