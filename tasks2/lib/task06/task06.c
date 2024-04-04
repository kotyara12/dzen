#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_bit_defs.h"
#include "esp_random.h"
#include "esp_log.h"
#include "task06.h"

const char* logTAG  = "TASK6";

#define APP_TASK_STACK_SIZE 4096
#define APP_TASK_PRIORITY   5
#define APP_TASK_CORE       1

// Номера GPIO, к которым подключены кнопки
#define GPIO_BUTTON1            GPIO_NUM_16
#define GPIO_BUTTON2            GPIO_NUM_17
#define GPIO_BUTTON3            GPIO_NUM_18

// Флаги
#define FLG_BUTTON1_PRESSED     BIT0
#define FLG_BUTTON2_PRESSED     BIT1
#define FLG_BUTTON3_PRESSED     BIT2

#define FLGS_BUTTONS            (FLG_BUTTON1_PRESSED | FLG_BUTTON2_PRESSED | FLG_BUTTON3_PRESSED)

// Статический буфер под служебные данные задачи
static StaticTask_t xTaskBuffer;
static StaticEventGroup_t xEventBuffer;
// Статический буфер под стек задачи
static StackType_t xStack[APP_TASK_STACK_SIZE];
// Указатель на группу событий
EventGroupHandle_t xEventGroup = NULL;

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

// Обработчик прерывания по нажатию кнопки
static void IRAM_ATTR isrButton1Press(void* arg)
{
  // Переменные для переключения контекста
  BaseType_t xHigherPriorityTaskWoken, xResult;
  xHigherPriorityTaskWoken = pdFALSE;
  // Поскольку мы "подписались" только на GPIO_INTR_NEGEDGE, мы уверены что это именно момент нажатия на кнопку
  xResult = xEventGroupSetBitsFromISR(xEventGroup, FLG_BUTTON1_PRESSED, &xHigherPriorityTaskWoken);
  // Если другая задача ждет этого события, передаем управление её досрочно (до завершения текущего тика ОС)
  if (xResult == pdPASS) {
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  };
}

// Настройка GPIO и прерываний
void gpioInit()
{
  // Устанавливаем сервис GPIO ISR service
  esp_err_t err = gpio_install_isr_service(0);
  if (err == ESP_ERR_INVALID_STATE) {
    ESP_LOGW("ISR", "GPIO isr service already installed");
  };

  // Настраиваем вывод для кнопки: активный уровень низкий со встроенной подяжкой к питанию
  gpio_reset_pin(GPIO_BUTTON1);
  gpio_set_direction(GPIO_BUTTON1, GPIO_MODE_INPUT);
  gpio_set_pull_mode(GPIO_BUTTON1, GPIO_PULLUP_ONLY);

  // Регистрируем обработчик прерывания на нажатие кнопки
  gpio_isr_handler_add(GPIO_BUTTON1, isrButton1Press, NULL);
  
  // Устанавливаем тип события для генерации прерывания - по низкому уровню
  gpio_set_intr_type(GPIO_BUTTON1, GPIO_INTR_NEGEDGE);
  
  // Разрешаем использование прерываний
  gpio_intr_enable(GPIO_BUTTON1);
}

// Функция задачи
void app_task_exec(void *pvParameters)
{
  uint8_t i = 0;
  EventBits_t uxBits;

  // Настройка GPIO и прерываний
  gpioInit();

  // Период ожидания 10 секунд "по умолчанию"
  TickType_t waitTicks = pdMS_TO_TICKS(10000);
  // Время последнего времени чтения данных с сенсоров
  TickType_t readTicks = xTaskGetTickCount();
  while(1) {
    // Ждем либо нажатия на кнопку, либо таймаута 10 секунд
    uxBits = xEventGroupWaitBits(
      xEventGroup,               // Указатель на группу событий
      FLGS_BUTTONS,              // Какие биты мы ждем
      pdTRUE,                    // Сбросить утановленные биты, после того как они были прочитаны
      pdFALSE,                   // Любой бит (даже один) приведет к выходу из ожидания
      waitTicks);                // Расчетный период ожидания в тиках

    // Проверяем, были ли нажатия на какую-либо кнопку
    if ((uxBits & FLG_BUTTON1_PRESSED) != 0) {
      // Была нажата кнопка 1, обрабатываем
      ESP_LOGI(logTAG, "Button 1 pressed");
    } 
    else if ((uxBits & FLG_BUTTON2_PRESSED) != 0) {
      // Была нажата кнопка 2, обрабатываем
      ESP_LOGI(logTAG, "Button 2 pressed");
    } 
    else if ((uxBits & FLG_BUTTON3_PRESSED) != 0) {
      // Была нажата кнопка 3, обрабатываем
      ESP_LOGI(logTAG, "Button 3 pressed");
    } 
    else {
      // Запоминаем время последнего чтения данных с сенсоров и выполения полезной работы
      readTicks = xTaskGetTickCount();

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
    };
    
    // Считаем время ожидания следующего цикла
    TickType_t currTicks = xTaskGetTickCount();    
    if ((currTicks - readTicks) >= pdMS_TO_TICKS(10000)) {
      // С момента последнего выполнения цикла измерений readTicks прошло больше времени, чем нужно
      waitTicks = 0;
    } else {
      // Из периода ожидания 10 секунд вычитаем то количество тиков, которое уже прошло с момента readTicks
      waitTicks = pdMS_TO_TICKS(10000) - (currTicks - readTicks);
    };
  };

  // Сюда мы не должны добраться никогда. Но если "что-то пошло не так" - нужно всё-таки удалить задачу из памяти
  vTaskDelete(NULL);
}

void app_task_start()
{
  // Создание группы с статическим выделением памяти
  xEventGroup = xEventGroupCreateStatic(&xEventBuffer);
  // Очищаем все рабочие биты группы
  xEventGroupClearBits(xEventGroup, 0x00FFFFFFU);
  // Запуск задачи с статическим выделением памяти
  TaskHandle_t xHandle = xTaskCreateStaticPinnedToCore(app_task_exec, "app_task", 
    APP_TASK_STACK_SIZE, NULL, APP_TASK_PRIORITY, xStack, &xTaskBuffer, APP_TASK_CORE);
  if (xHandle == NULL) {
    ESP_LOGE(logTAG, "Failed to task create");
  } else {
    ESP_LOGI(logTAG, "Task started");
  };
}

