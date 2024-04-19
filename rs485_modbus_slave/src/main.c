#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "mbcontroller.h"

// Указатель на обработчик протокола modbus
void* modbus_slave = NULL;

// Количество регистров подряд
#define MB_REG_HOLDING_CNT    (16)                   
// Адрес первого регистра в области
#define MB_REG_HOLDING_START  (0x0000)                

// Массив для хранения данных регистров
uint16_t holding_reg_area[MB_REG_HOLDING_CNT] = {0}; 

// Мьютекс для защиты области хранения данных регистров от изменения из другой задачи
static portMUX_TYPE holding_reg_area_lock = portMUX_INITIALIZER_UNLOCKED;

/************************************************************
// Чтение или изменение данных в массиве из другой задачи
void task_exec(void* param) 
{
  ...
  portENTER_CRITICAL(&holding_reg_area_lock);
  holding_reg_area[2] = 10;
  portEXIT_CRITICAL(&holding_reg_area_lock);
  ...
}
*************************************************************/

void app_main() 
{
  // Инициализация RS485 и Modbus в режиме slave
  ESP_ERROR_CHECK(mbc_slave_init(MB_PORT_SERIAL_SLAVE, &modbus_slave));
  
  // Настраиваем Modbus
  mb_communication_info_t comm;
  memset(&comm, 0, sizeof(comm));
  comm.slave_addr = 1;                // Адрес устройства
  comm.mode = MB_MODE_RTU;            // Режим Modbus RTU
  comm.port = UART_NUM_1;             // Порт UART1
  comm.baudrate = 9600;               // Скорость 9600
  comm.parity = UART_PARITY_DISABLE;  // Контроль четности отключена
  ESP_ERROR_CHECK(mbc_slave_setup((void*)&comm));
  
  // Настраиваем выводы UART: TX=16, RX=17, RTS и CTS не используются
  ESP_ERROR_CHECK(uart_set_pin(UART_NUM_1, 16, 17, -1, -1));
  
  // Указываем области регистров, которые будет обслуживать slave устройство
  mb_register_area_descriptor_t reg_area;
  reg_area.type = MB_PARAM_HOLDING;                    // Тип регистров в области
  reg_area.start_offset = MB_REG_HOLDING_START;        // Начальный адрес области в протоколе Modbus
  reg_area.address = (void*)&holding_reg_area[0];      // Указатель на массив данных, в которых хранятся данные регистров
  reg_area.size = sizeof(holding_reg_area);            // Размер области хранения данных в байтах
  ESP_ERROR_CHECK(mbc_slave_set_descriptor(reg_area));

  // Запускаем Modbus
  ESP_ERROR_CHECK(mbc_slave_start());
  
  // Настраиваем режим HALF_DUPLEX
  ESP_ERROR_CHECK(uart_set_mode(UART_NUM_1, UART_MODE_RS485_HALF_DUPLEX));

  // Сообщение в лог
  ESP_LOGI("RS485", "Modbus slave stack initialized");

  // Цикл обработки запросов от мастера
  mb_param_info_t reg_info;
  while (1) {
    // Ждем событий чтения или записи регистра от ведущего с блокировкой цикла
    mb_event_group_t event = mbc_slave_check_event(MB_EVENT_HOLDING_REG_WR | MB_EVENT_HOLDING_REG_RD);
    // Получаем данные о регистре
    ESP_ERROR_CHECK_WITHOUT_ABORT(mbc_slave_get_param_info(&reg_info, pdMS_TO_TICKS(100)));
    // Что-то делаем... например считаем количество обращений мастера к любым регистрам
    portENTER_CRITICAL(&holding_reg_area_lock);
    holding_reg_area[0] += 1;
    portEXIT_CRITICAL(&holding_reg_area_lock);
  };
}