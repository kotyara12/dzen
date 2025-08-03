#include <stdio.h>
#include "esp_log.h"
// Подключаемые библиотеки
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_types.h"
#include "driver/i2c_master.h"

const char* logTAG = "I2C";

void app_main(void)
{
  // ==========================================================
  // Настройка и запуск шины I2C #0
  // ==========================================================
  // Задаем параметры шины I2C
  i2c_master_bus_config_t i2c_master_config;
  i2c_master_config.clk_source = I2C_CLK_SRC_DEFAULT; // Источник синхронизации для шины
  i2c_master_config.i2c_port = I2C_NUM_0;             // Номер шины (I2C_NUM_0 или I2C_NUM_1)
  i2c_master_config.scl_io_num = 22;                  // Номер GPIO для линии синхронизации SCL
  i2c_master_config.sda_io_num = 21;                  // Номер GPIO для линии данных SDА
  i2c_master_config.flags.enable_internal_pullup = 1; // Использовать встроенную подяжку GPIO
  i2c_master_config.glitch_ignore_cnt = 7;            // Период сбоя данных на шине, стандартное значение 7
  i2c_master_config.intr_priority = 0;                // Приоритет прерывания: авто
  i2c_master_config.trans_queue_depth = 0;            // Глубина внутренней очереди. Действительно только при асинхронной передаче

  // Настраиваем шину
  i2c_master_bus_handle_t bus_handle = NULL;
  ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_master_config, &bus_handle)); 
  ESP_LOGI(logTAG, "I2C bus is configured");

  // ==========================================================
  // Сканирование шины (поиск устройств)
  // ==========================================================
  for (uint8_t i = 1; i < 128; i++) {
    if (i2c_master_probe(bus_handle, i, -1) == ESP_OK) {
      ESP_LOGI(logTAG, "Found device on bus 0 at address 0x%.2X", i);
    };
  };

  // ==========================================================
  // Настройка slave-устройства
  // ==========================================================
  // Задаем параметры slave-устройства
  i2c_device_config_t i2c_device_config;
  i2c_device_config.dev_addr_length = I2C_ADDR_BIT_LEN_7; // Используется стандартная 7-битная адресация
  i2c_device_config.device_address = 0x38;                // Адрес устройства
  i2c_device_config.scl_speed_hz = 100000;                // Скорость шины 100kHz
  i2c_device_config.scl_wait_us = 0;                      // Время ожидания по умолчанию
  i2c_device_config.flags.disable_ack_check = 0;          // Не отключать проверку ACK

  // Настраиваем подчиненное устройство
  i2c_master_dev_handle_t dev_handle;
  ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &i2c_device_config, &dev_handle));
  i2c_master_register_event_callbacks(dev_handle, );
  ESP_LOGI(logTAG, "Slave device is configured");

  // Буферы под отправку команд и получение данных от сенсора
  uint8_t status = 0;
  uint8_t bufCmd[3] = {0, 0, 0};
  uint8_t bufData[7] = {0, 0, 0, 0, 0, 0, 0};

  // Настраиваем сенсор AHT20
  bufCmd[0] = 0xBE; // Команда инициализации
  bufCmd[1] = 0x08; // Загрузить калибровочные коэффициенты из EEPROM
  bufCmd[2] = 0x00; // NOP control
  ESP_ERROR_CHECK(i2c_master_transmit(dev_handle, &bufCmd[0], 3, -1));
  ESP_LOGI(logTAG, "AHT20 is configured");
  vTaskDelay(pdMS_TO_TICKS(100));

  // ==========================================================
  // Основной цикл задачи
  // ==========================================================
  while (1) {
    // Отправляем команду на измерение
    bufCmd[0] = 0xAC; // Запуск измерения
    bufCmd[1] = 0x33; // Подозреваю, что это разрешение ЦАП температуры и влажности
    bufCmd[2] = 0x00; // NOP control
    ESP_ERROR_CHECK(i2c_master_transmit(dev_handle, &bufCmd[0], 3, -1));

    // Ожидание, пока AHT20 занят измерением ( >75 мс )
    vTaskDelay(pdMS_TO_TICKS(75));
    ESP_ERROR_CHECK(i2c_master_receive(dev_handle, &status, 1, -1));
    while ((status != 0xFF) && (status & 0x80)) {
      vTaskDelay(pdMS_TO_TICKS(5));
      ESP_ERROR_CHECK(i2c_master_receive(dev_handle, &status, 1, -1));
    };

    // Чтение результата: {status, RH, RH, RH+T, T, T, CRC}
    ESP_ERROR_CHECK(i2c_master_receive(dev_handle, &bufData[0], 7, -1));

    // Декодируем и выводим результаты измерений (!проверку CRC я для краткости опустил!)
    uint32_t hData = (((uint32_t)bufData[1] << 16) | ((uint16_t)bufData[2] << 8) | (bufData[3])) >> 4;
    float hValue = ((float)hData / 0x100000) * 100.0;

    uint32_t tData = ((uint32_t)(bufData[3] & 0x0F) << 16) | ((uint16_t)bufData[4] << 8) | bufData[5]; 
    float tValue = ((float)tData / 0x100000) * 200.0 - 50.0;

    ESP_LOGI(logTAG, "AHT20 temperature = %f, humidity = %f", tValue, hValue);

    // Ожидание 1000 мс
    vTaskDelay(pdMS_TO_TICKS(1000));
  };
}
