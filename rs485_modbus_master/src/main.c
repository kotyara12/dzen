#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "mbcontroller.h"

// Указатель на обработчик протокола modbus
void* modbus_master = NULL;

// Структура данных для хранения данных регистров
#pragma pack(push, 1)
typedef struct
{
  uint16_t humidity;
  uint16_t temperature;
  uint16_t address;
} holding_reg_params_t;
#pragma pack(pop)
holding_reg_params_t holding_reg_params = {0};

// Макросы для получения смещения параметра в соответствующей структуре.
#define HOLD_OFFSET(field) ((uint16_t)(offsetof(holding_reg_params_t, field) + 1))
// Макрос для объявления строк
#define STR(fieldname) ((const char*)(fieldname))
// Параметры могут использоваться как битовые маски или ограничения параметров
#define OPTS(min_val, max_val, step_val) { .min = min_val, .max = max_val, .step = step_val }


// Перечисление всех возможных регистров (параметров)
enum {
  CID_HOLD_HUMIDITY = 0,
  CID_HOLD_TEMPERATURE,
  CID_HOLD_ADDRESS,
  CID_COUNT
};

// Описание таблицы параметров (для всех slave-устройств сразу)
const mb_parameter_descriptor_t device_parameters[] = {
    // { CID, Param Name, Units, Modbus Slave Addr, Modbus Reg Type, Reg Start, Reg Size, Instance Offset, Data Type, Data Size, Parameter Options, Access Mode}
    {CID_HOLD_HUMIDITY, STR("Humidity"), STR("%rH"), 0x01, MB_PARAM_HOLDING, 0x0000, 1, 
      HOLD_OFFSET(humidity), PARAM_TYPE_U16, PARAM_SIZE_U16, OPTS(0,0,0), PAR_PERMS_READ},
    {CID_HOLD_TEMPERATURE, STR("Temperature"), STR("C"), 0x01, MB_PARAM_HOLDING, 0x0001, 1, 
      HOLD_OFFSET(temperature), PARAM_TYPE_U16, PARAM_SIZE_U16, OPTS(0,0,0), PAR_PERMS_READ},
    {CID_HOLD_ADDRESS, STR("Address"), STR("-"), 0x01, MB_PARAM_HOLDING, 0x0100, 1, 
      HOLD_OFFSET(address), PARAM_TYPE_U16, PARAM_SIZE_U16, OPTS(0,255,1), PAR_PERMS_READ_WRITE},
};

// Вычисление количества параметров в таблице
const uint16_t num_device_parameters = (sizeof(device_parameters)/sizeof(device_parameters[0]));

//  Настраиваем Modbus в master-режиме через порт UART1
void init_modbus_master()
{
  // Инициализация RS485 и Modbus
  ESP_ERROR_CHECK(mbc_master_init(MB_PORT_SERIAL_MASTER, &modbus_master));
  
  // Настраиваем Modbus
  mb_communication_info_t comm;
  memset(&comm, 0, sizeof(comm));
  comm.mode = MB_MODE_RTU;            // Режим Modbus RTU
  comm.port = UART_NUM_1;             // Порт UART1
  comm.baudrate = 9600;               // Скорость 9600
  comm.parity = UART_PARITY_DISABLE;  // Контроль четности отключена
  ESP_ERROR_CHECK(mbc_master_setup((void*)&comm));
  
  // Настраиваем выводы UART: TX=16, RX=17, RTS и CTS не используются
  ESP_ERROR_CHECK(uart_set_pin(UART_NUM_1, 16, 17, -1, -1));
  
  // Запускаем Modbus
  ESP_ERROR_CHECK(mbc_master_start());
  
  // Настраиваем режим HALF_DUPLEX
  ESP_ERROR_CHECK(uart_set_mode(UART_NUM_1, UART_MODE_RS485_HALF_DUPLEX));

  // Регистрируем таблицу параметров
  ESP_ERROR_CHECK(mbc_master_set_descriptor(&device_parameters[0], num_device_parameters));
}

// Читаем характеристики по таблице
void read_modbus_parameters()
{
  uint8_t type = 0;
  // Указатель на запись таблицы параметров
  const mb_parameter_descriptor_t* param_descriptor = NULL;

  // Запрашиваем все возможные параметры
  for (uint16_t cid = 0; cid < CID_COUNT; cid++) {
    // Получаем указатель на запись таблицы параметров
    esp_err_t err = mbc_master_get_cid_info(cid, &param_descriptor);
    if ((err != ESP_ERR_NOT_FOUND) && (param_descriptor != NULL)) {
      // Получаем указатель на хранилище данных параметра: указатель на глобальную переменную + смещение - 1
      void* temp_data_ptr = (void*)&holding_reg_params + param_descriptor->param_offset - 1;
      // Читаем данные параметра
      err = mbc_master_get_parameter(param_descriptor->cid, (char*)param_descriptor->param_key, (uint8_t*)temp_data_ptr, &type);
      // Выводим на экран
      if (err == ESP_OK) {
        ESP_LOGI("RS485", "Characteristic #%d %s (%s) value = %d read successful.",
          param_descriptor->cid, (char*)param_descriptor->param_key, (char*)param_descriptor->param_units, *(int16_t*)temp_data_ptr);
      } else {
        ESP_LOGE("RS485", "Characteristic #%d (%s) read fail, err = 0x%x (%s).",
          param_descriptor->cid, (char*)param_descriptor->param_key, (int)err, (char*)esp_err_to_name(err));
      };
    };
  };
}

void app_main() 
{
  init_modbus_master();
  while (1) {
    read_modbus_parameters();
    vTaskDelay(pdMS_TO_TICKS(3000));
  }
}