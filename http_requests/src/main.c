#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_http_client.h"
#include "esp_log.h"

#define STACK_SIZE 4096

const char* TAG = "HTTP";

// Функция задачи
void led_exec(void *pvParameters)
{
  // Параметры конфигурации HTTP-соединения
  esp_http_client_config_t request;
  memset(&request, 0, sizeof(request));

  // Транспорт TCP/IP
  request.transport_type = HTTP_TRANSPORT_OVER_TCP;
  // Запрос типа GET
  request.method = HTTP_METHOD_GET;
  // Блокировка задачи на время выполнения обмена с серверос
  request.is_async = false;
  // Закрыть соединение сразу после отправки всех данных
  request.keep_alive_enable = false; 
  // Таймаут передачи
  request.timeout_ms = 60000;
  // Разрешить автоматическую переадресацию без ограничений
  request.disable_auto_redirect = false;
  request.max_redirection_count = 0;

  // Инициализируем соединение
  esp_http_client_handle_t client = esp_http_client_init(&request);
  if (client) {
    // Рабочий цикл задачи
    while(1) {
      // Устанавливаем новый URL (для простоты я все равно не буду тут ничего изобретать разного)
      esp_http_client_set_url(client, "http://open-monitoring.online/get?cid=2468&key=H9xOdR&p1=2&p2=hello&p3=99.99");
      // Выполняем запрос
      esp_http_client_perform(client);
      // Анализ результатов
      int response = esp_http_client_get_status_code(client);
      if (response != 200) {
        ESP_LOGE(TAG, "Oops! Something went wrong: %d", response);
      };

      // Пауза 1 минута
      vTaskDelay(pdMS_TO_TICKS(60000));
    };

    // У нас порядок простой - поел, убери за собой!
    esp_http_client_cleanup(client);
  };
  vTaskDelete(NULL);
}

void app_main() 
{
  static StaticTask_t xTaskBuffer;
  static StackType_t xStack[STACK_SIZE];

  TaskHandle_t xHandle = xTaskCreateStaticPinnedToCore(led_exec, "http_request", STACK_SIZE, NULL, 5, xStack, &xTaskBuffer, 1);
  if (xHandle == NULL) {
    ESP_LOGE("TASK", "Failed to task create");
  };
}