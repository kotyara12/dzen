#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"
#include "esp_heap_caps.h"
#include "esp_http_client.h"
#include "esp_tls.h"
#include "esp_crt_bundle.h"

#define STACK_SIZE 4096

const char* TAG = "HTTP";

// Функция задачи
void task_exec(void *pvParameters)
{
  // Статический буфер под строку запроса с переменными данными
  static char buffer[256];

  // Параметры конфигурации HTTP-соединения
  esp_http_client_config_t request;
  memset(&request, 0, sizeof(request));

  // Устанавливаем URL "по умолчанию", в дальнейшем он будет заменен другим актуальным значением, поэтому можно использовать любой
  request.url = "open-monitoring.online/get?cid=9999&key=LH9rR3";
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

  // Настройка параметров защищенного соединения TLS
  // Транспорт TCP/IP over SSL
  request.transport_type = HTTP_TRANSPORT_OVER_SSL;
  request.skip_cert_common_name_check = false;
  // Подключаем GLOBAL STORE
  request.use_global_ca_store = false;
  request.crt_bundle_attach = esp_crt_bundle_attach;

  uint16_t step = 0;
  size_t heap_size = heap_caps_get_total_size(MALLOC_CAP_DEFAULT);
  size_t heap_start = 0;

  // Инициализируем соединение
  esp_http_client_handle_t client = esp_http_client_init(&request);
  if (client) {
    // Рабочий цикл задачи
    while(1) {
      // Пауза ~1 минута
      vTaskDelay(pdMS_TO_TICKS(61000));

      size_t heap_free = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
      ESP_LOGI("RAM", "Heap total: %d, usage: %d", heap_size, heap_size-heap_free);
      if (heap_start == 0) {
        heap_start = heap_free;
      };

      // Генерируем запрос в буфере
      memset(buffer, 0, sizeof(buffer));
      snprintf(buffer, sizeof(buffer), 
        "open-monitoring.online/get?cid=9999&key=LH9rR3&p1=%d&p2=%d&p3=%d&p4=%d", 
          heap_size, heap_size-heap_free, heap_size-heap_start, step);
      esp_http_client_set_url(client, buffer);
      ESP_LOGI(TAG, "Send HTTP request: %s", buffer);

      // Выполняем запрос
      esp_http_client_perform(client);
      esp_http_client_close(client);
      // Анализ результатов
      int response = esp_http_client_get_status_code(client);
      ESP_LOGI(TAG, "esp_http_client_get_status_code = %d", response);
      
      step++;
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
  TaskHandle_t xHandle = xTaskCreateStaticPinnedToCore(task_exec, "http_request", STACK_SIZE, NULL, 5, xStack, &xTaskBuffer, 1);
  if (xHandle == NULL) {
    ESP_LOGE("TASK", "Failed to task create");
  };
}