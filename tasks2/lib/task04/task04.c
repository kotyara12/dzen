#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_random.h"
#include "esp_log.h"
#include "task04.h"

const char* logTAG  = "TASK4";

#define APP_TASK_STACK_SIZE 4096
#define APP_TASK_PRIORITY   5
#define APP_TASK_CORE       1
#define APP_QUEUE_LENGTH    16
#define APP_QUEUE_ITEM_SIZE sizeof(float)

// Статический буфер под служебные данные задачи и очереди
static StaticTask_t xTaskBuffer;
static StaticQueue_t xQueueBuffer;
// Статический буфер под стек задачи и очереди
static StackType_t xStack[APP_TASK_STACK_SIZE];
static uint8_t xQueueStorage[APP_QUEUE_LENGTH * APP_QUEUE_ITEM_SIZE];
// Указатель на очередь
static QueueHandle_t dataQueue = NULL;

// Помещаем указатель на строку в очередь данной задачи
void insertStringIntoQueue(char* text)
{
  xQueueSend(dataQueue, &text, portMAX_DELAY);
}

// Отправка результатов измерений куда-то на сервер
void sendHttpRequest(char* text)
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
  char* text = NULL;

  // Бесконечный цикл задачи
  while(1) {
    // Ждем данные извне
    while (xQueueReceive(dataQueue, &text, portMAX_DELAY) == pdPASS) {
      if (text != NULL) {
        // Выводим сообщение в терминал
        ESP_LOGI(logTAG, "Text recieved: %s", text);

        // Отправляем данные на сервер
        sendHttpRequest(text);

        // И не забываем удалить текст из памяти
        free(text);
      };
    };
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
    // Создаем входящую очередь
    dataQueue = xQueueCreateStatic(APP_QUEUE_LENGTH, APP_QUEUE_ITEM_SIZE, &xQueueStorage[0], &xQueueBuffer);
    ESP_LOGI(logTAG, "Task started");
  };
}

