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
#if CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
#include "esp_crt_bundle.h"
#endif // CONFIG_MBEDTLS_CERTIFICATE_BUNDLE

#define STACK_SIZE 4096

const char* TAG = "HTTP";

// Константомакросы
#define API_REQUEST_BASE "open-monitoring.online/get?cid=2481&key=rR3LH9"
#define API_INTERVAL_MS  65000

//*******************************************************************************************
// Объявляем указатели на внешний двоичный файл с файлом корневого сертификата
// !!! Имеет смысл только если не используется CERTIFICATE BUNDLE
//*******************************************************************************************
#ifndef CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
  extern const char api_isrg_root_x1_start[] asm("_binary_isrg_root_x1_pem_start");
  extern const char api_isrg_root_x1_end[]   asm("_binary_isrg_root_x1_pem_end");
#endif // CONFIG_MBEDTLS_CERTIFICATE_BUNDLE

//*******************************************************************************************
// Формирование шаблона HTTP-запроса
//*******************************************************************************************
#if CONFIG_CONFIG_HTTP_REUSE_CONNECTION
  #define _P10_VALUE "1"
#else
  #define _P10_VALUE "0"
#endif

#if CONFIG_CONFIG_HTTP_SSL_ENABLED
  #if CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
    #define _P4_MODE "TLS%%20bundle"
  #else
    #if CONFIG_CONFIG_HTTP_SSL_GLOBAL_STORE
      #define _P4_MODE "TLS%%20global%%20store"
    #else
      #define _P4_MODE "TLS%%20buffer"
    #endif // CONFIG_CONFIG_HTTP_SSL_GLOBAL_STORE
  #endif // CONFIG_MBEDTLS_CERTIFICATE_BUNDLE

  #if CONFIG_MBEDTLS_SSL_KEEP_PEER_CERTIFICATE
    #define _P6_VALUE "1"
  #else
    #define _P6_VALUE "0"
  #endif
  #if CONFIG_MBEDTLS_SSL_CONTEXT_SERIALIZATION
    #define _P7_VALUE "1"
  #else
    #define _P7_VALUE "0"
  #endif
  #if CONFIG_MBEDTLS_SSL_VARIABLE_BUFFER_LENGTH
    #define _P8_VALUE "1"
  #else
    #define _P8_VALUE "0"
  #endif
  #if CONFIG_MBEDTLS_DYNAMIC_BUFFER
    #define _P9_VALUE "1"
  #else
    #define _P9_VALUE "0"
  #endif
  #define OPENMON_QUERY "https://" API_REQUEST_BASE "&p1=%d&p2=%d&p3=%d&p4=" _P4_MODE "&p5=" IDF_VER "&p6=" _P6_VALUE "&p7=" _P7_VALUE "&p8=" _P8_VALUE "&p9=" _P9_VALUE "&p10=" _P10_VALUE "&p11=%d"
#else
  #define _P4_MODE "NO%%20TLS"
  #define OPENMON_QUERY "http://" API_REQUEST_BASE "&p1=%d&p2=%d&p3=%d&p4=" _P4_MODE "&p5=" IDF_VER "&p10=" _P10_VALUE "&p11=%d"
#endif // CONFIG_CONFIG_HTTP_SSL_ENABLED 

//*******************************************************************************************
// Функция задачи
//*******************************************************************************************
void task_exec(void *pvParameters)
{
  // Статический буфер под строку запроса с переменными данными
  static char buffer[256];

  // Параметры конфигурации HTTP-соединения
  esp_http_client_config_t request;
  memset(&request, 0, sizeof(request));

  // Устанавливаем URL "по умолчанию", в дальнейшем он будет заменен другим актуальным значением, поэтому можно использовать любой
  request.url = OPENMON_QUERY;
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
  #if CONFIG_CONFIG_HTTP_SSL_ENABLED
    // Транспорт TCP/IP over SSL
    request.transport_type = HTTP_TRANSPORT_OVER_SSL;
    request.skip_cert_common_name_check = false;
    // Будет использован CERTIFICATE BUNDLE
    #if CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
      request.use_global_ca_store = false;
      request.crt_bundle_attach = esp_crt_bundle_attach;
    #else
      // Будет использован GLOBAL STORE
      #if CONFIG_CONFIG_HTTP_SSL_GLOBAL_STORE
        request.use_global_ca_store = true;
        request.crt_bundle_attach = NULL;
      #else
        // Прямое указание на корневой сертификат
        request.cert_pem = api_isrg_root_x1_start;
        request.cert_len = api_isrg_root_x1_end - api_isrg_root_x1_start;
        request.use_global_ca_store = false;
        request.crt_bundle_attach = NULL;
      #endif // CONFIG_CONFIG_HTTP_SSL_GLOBAL_STORE
    #endif // CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
  #else
    // Транспорт TCP/IP, шифрование отключено
    request.transport_type = HTTP_TRANSPORT_OVER_TCP;
  #endif // CONFIG_CONFIG_HTTP_SSL_ENABLED

  uint16_t step = 0;
  size_t heap_size = heap_caps_get_total_size(MALLOC_CAP_DEFAULT);
  size_t heap_start = 0;

  #if CONFIG_CONFIG_HTTP_REUSE_CONNECTION
    /***************************************************************************************
     * Повторное использование HTTP соединения
     ***************************************************************************************/
    
    // Инициализируем соединение
    esp_http_client_handle_t client = esp_http_client_init(&request);
    if (client) {
      // Рабочий цикл задачи
      while(1) {
        // Пауза ~1 минута
        vTaskDelay(pdMS_TO_TICKS(API_INTERVAL_MS));

        size_t heap_free = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
        ESP_LOGI("RAM", "Heap total: %d, usage: %d", heap_size, heap_size-heap_free);
        if (heap_start == 0) {
          heap_start = heap_free;
        };

        // Генерируем запрос в буфере
        memset(buffer, 0, sizeof(buffer));
        snprintf(buffer, sizeof(buffer), OPENMON_QUERY, heap_size, heap_size-heap_free, heap_size-heap_start, step);
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
  #else
    /***************************************************************************************
     * Однократное HTTP соединение
     ***************************************************************************************/
    
    // Рабочий цикл задачи
    while(1) {
      // Пауза ~1 минута
      vTaskDelay(pdMS_TO_TICKS(API_INTERVAL_MS));

      size_t heap_free = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
      ESP_LOGI("RAM", "Heap total: %d, usage: %d", heap_size, heap_size-heap_free);
      if (heap_start == 0) {
        heap_start = heap_free;
      };

      // Генерируем запрос в буфере
      memset(buffer, 0, sizeof(buffer));
      snprintf(buffer, sizeof(buffer), OPENMON_QUERY, heap_size, heap_size-heap_free, heap_size-heap_start, step);
      request.url = buffer;
      ESP_LOGI(TAG, "Send HTTP request: %s", buffer);

      // Инициализируем соединение
      esp_http_client_handle_t client = esp_http_client_init(&request);
      if (client) {
        // Выполняем запрос
        esp_http_client_perform(client);
        // Анализ результатов
        int response = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "esp_http_client_get_status_code = %d", response);
        // У нас порядок простой - поел, убери за собой!
        esp_http_client_cleanup(client);
      };
      
      step++;
    };
  #endif

  vTaskDelete(NULL);
}

void app_main() 
{
  // Подключение к сети WiFi
  ESP_ERROR_CHECK(nvs_flash_init());
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  ESP_ERROR_CHECK(example_connect()); 

  // Если используется GLOBAL STORE, то инициализируем его
  #if CONFIG_CONFIG_HTTP_SSL_ENABLED && CONFIG_CONFIG_HTTP_SSL_GLOBAL_STORE
    esp_tls_init_global_ca_store();
    // Добавим наш сертификат в GLOBAL STORE
    esp_tls_set_global_ca_store((const unsigned char*)api_isrg_root_x1_start, api_isrg_root_x1_end-api_isrg_root_x1_start);
  #endif // CONFIG_CONFIG_HTTP_SSL_GLOBAL_STORE

  // Запускаем задачу HTTP
  static StaticTask_t xTaskBuffer;
  static StackType_t xStack[STACK_SIZE];
  TaskHandle_t xHandle = xTaskCreateStaticPinnedToCore(task_exec, "http_request", STACK_SIZE, NULL, 5, xStack, &xTaskBuffer, 1);
  if (xHandle == NULL) {
    ESP_LOGE("TASK", "Failed to task create");
  };
}