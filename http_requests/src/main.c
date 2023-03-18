#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"
#include "esp_log.h"
#include "esp_heap_caps.h"

#include "esp_http_client.h"

#define STACK_SIZE 4096

const char* TAG = "HTTP";

// Функция задачи
void led_exec(void *pvParameters)
{
  // Параметры конфигурации HTTP-соединения
  esp_http_client_config_t request;
  memset(&request, 0, sizeof(request));

  // Начальный URI
  request.url = "http://open-monitoring.online/get?cid=2468&key=H9xOdR";
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
      // Закрываем соединение, если оно открыто
      esp_http_client_close(client);
      // Анализ результатов
      int response = esp_http_client_get_status_code(client);
      ESP_LOGI(TAG, "esp_http_client_get_status_code = %d", response);

      // Выводим информацию о свободной памяти
      double heap_total = (double)heap_caps_get_total_size(MALLOC_CAP_DEFAULT) / 1024.0;
      double heap_free  = (double)heap_caps_get_free_size(MALLOC_CAP_DEFAULT) / 1024.0;
      double heap_min   = (double)heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT) / 1024.0;
      ESP_LOGI("RAM", "Heap total: %.3f kB, free: %.3f kB (%.1f%%), minimum: %.3f kB (%.1f%%)",
         heap_total, heap_free, 100.0*heap_free/heap_total, heap_min, 100.0*heap_min/heap_total);

      // Пауза 3 минуты
      vTaskDelay(pdMS_TO_TICKS(180000));
    };

    // У нас порядок простой - поел, убери за собой!
    esp_http_client_cleanup(client);
  };
  vTaskDelete(NULL);
}

void app_main() 
{
  // Подключение к сети WiFi
  ESP_ERROR_CHECK(nvs_flash_init());
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  ESP_ERROR_CHECK(example_connect()); 

  // Запускаем задачу HTTP
  static StaticTask_t xTaskBuffer;
  static StackType_t xStack[STACK_SIZE];

  TaskHandle_t xHandle = xTaskCreateStaticPinnedToCore(led_exec, "http_request", STACK_SIZE, NULL, 5, xStack, &xTaskBuffer, 1);
  if (xHandle == NULL) {
    ESP_LOGE("TASK", "Failed to task create");
  };
}