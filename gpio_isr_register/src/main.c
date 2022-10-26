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

static xQueueHandle gpio_queue = NULL;

// Обработчик прерывания по нажатию кнопки
static void IRAM_ATTR isrGPIO(void* arg)
{
  // Переменные для переключения контекста
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  BaseType_t xResult = pdFALSE;

  // Считываем и сбрасываем регистры прерываний GPIO
  uint32_t gpio_intr_status = READ_PERI_REG(GPIO_STATUS_REG);     // Read status to get interrupt status for GPIO0-31
  SET_PERI_REG_MASK(GPIO_STATUS_W1TC_REG, gpio_intr_status);      // Clear interrupt status for GPIO0-31

  uint32_t gpio_intr_status_h = READ_PERI_REG(GPIO_STATUS1_REG);  // Read status to get interrupt status for GPIO32-39
  SET_PERI_REG_MASK(GPIO_STATUS1_W1TC_REG, gpio_intr_status_h);   // Clear interrupt status for GPIO32-39

  // Определяем номер порта, с которого поступило событие
  uint32_t gpio_num = 0;
  do {
    if (gpio_num < 32) {
      if (gpio_intr_status & BIT(gpio_num)) { 
        // Отправляем в очередь задачи номер нажатой кнопки
        xResult = xQueueSendFromISR(gpio_queue, &gpio_num, &xHigherPriorityTaskWoken);
		  }
		} else {
      if (gpio_intr_status_h & BIT(gpio_num - 32)) {
        // Отправляем в очередь задачи номер нажатой кнопки
        xResult = xQueueSendFromISR(gpio_queue, &gpio_num, &xHigherPriorityTaskWoken);
      }
    };
  } while (++gpio_num < GPIO_PIN_COUNT);
  
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
  uint32_t gpio_num;

  while(1) {
    // Ждем события в очереди
    if (xQueueReceive(gpio_queue, &gpio_num, portMAX_DELAY)) {
      // Если это кнопка...
      if (gpio_num == 18) {
        ESP_LOGI("ISR", "Button is pressed");
        // Переключаем светодиод
        led_state = !led_state;
        gpio_set_level(GPIO_NUM_16, (uint32_t)led_state);
      };
    };
  };
  vTaskDelete(NULL);
}

void app_main() 
{
  // Создаем входящую очередь задачи
  gpio_queue = xQueueCreate(32, sizeof(uint32_t));

  // Запускаем задачу управления светодиодом
  xTaskCreatePinnedToCore(led_exec, "led", 4096, NULL, 5, NULL, 1);

  // Настраиваем вывод для кнопки
  gpio_pad_select_gpio(18);
  gpio_set_direction(GPIO_NUM_18, GPIO_MODE_INPUT);
  gpio_set_pull_mode(GPIO_NUM_18, GPIO_FLOATING);

  // Регистрируем обработчик прерываний
  gpio_isr_register(isrGPIO, NULL, 0, NULL);
  
  // Устанавливаем тип события для генерации прерывания - по низкому уровню
  gpio_set_intr_type(GPIO_NUM_18, GPIO_INTR_NEGEDGE);
  
  // Разрешаем использование прерываний
  gpio_intr_enable(GPIO_NUM_18);
}