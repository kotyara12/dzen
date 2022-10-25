#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"

/**********************************************
 * СХЕМА СОЕДИНЕНИЙ:
 * 
 * На GPIO 18 подключена кнопка: одним выводом на GPIO 18, другим на землю. 
 * GPIO 18 подтянут резистором 10кОм на шину питания +3,3В (но можно использовать и внутреннюю подтяжку).
 * 
 * На GPIO 16 подключен светодиод (как в предыдущем примере) - одним выводом на +3,3В, 
 * другим - к GPIO 16 через резистор
 * 
 ***********************************************/

static xQueueHandle button_queue = NULL;

// Обработчик прерывания по нажатию кнопки
static void IRAM_ATTR isrButtonPress(void* arg)
{
  // Переменные для переключения контекста
  BaseType_t xHigherPriorityTaskWoken, xResult;
  xHigherPriorityTaskWoken = pdFALSE;
  // Поскольку мы "подписались" только на GPIO_INTR_NEGEDGE, мы уверены что это именно момент нажатия на кнопку
  bool pressed = true;
  // Отправляем в очередь задачи событие "кнопка нажата"
  xResult = xQueueSendFromISR(button_queue, &pressed, &xHigherPriorityTaskWoken);
  // Если высокоприоритетная задача ждет этого события, переключаем управление
  if (xResult == pdPASS) {
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  };
}

// Функция задачи светодиода
void led_exec(void *pvParameters)
{
  // Настраиваем вывод GPIO_NUM_16 на выход без подтяжки
  gpio_pad_select_gpio(16);
  gpio_set_direction(GPIO_NUM_16, GPIO_MODE_OUTPUT_OD);
  gpio_set_pull_mode(GPIO_NUM_16, GPIO_FLOATING);

  // Управление светодиодом
  bool led_state = false;
  bool led_pressed;

  while(1) {
    // Ждем события нажатия кнопки в очереди
    if (xQueueReceive(button_queue, &led_pressed, portMAX_DELAY)) {
      ESP_LOGI("ISR", "Button is pressed");
      // Переключаем светодиод
      led_state = !led_state;
      gpio_set_level(GPIO_NUM_16, (uint32_t)led_state);
    };
  };
  vTaskDelete(NULL);
}

void app_main() 
{
  // Создаем входящую очередь задачи
  button_queue = xQueueCreate(32, sizeof(bool));

  // Запускаем задачу управления светодиодом
  xTaskCreatePinnedToCore(led_exec, "led", 4096, NULL, 5, NULL, 1);

  // Настраиваем вывод для кнопки
  gpio_pad_select_gpio(18);
  gpio_set_direction(GPIO_NUM_18, GPIO_MODE_INPUT);
  gpio_set_pull_mode(GPIO_NUM_18, GPIO_FLOATING);

  // Устанавливаем сервис GPIO ISR service
  esp_err_t err = gpio_install_isr_service(0);
  if (err == ESP_ERR_INVALID_STATE) {
    ESP_LOGW("ISR", "GPIO isr service already installed");
  };

  // Регистрируем обработчик прерывания на нажатие кнопки
  gpio_isr_handler_add(GPIO_NUM_18, isrButtonPress, NULL);
  
  // Устанавливаем тип события для генерации прерывания - по низкому уровню
  gpio_set_intr_type(GPIO_NUM_18, GPIO_INTR_NEGEDGE);
  
  // Разрешаем использование прерываний
  gpio_intr_enable(GPIO_NUM_18);
}