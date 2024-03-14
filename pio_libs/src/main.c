// Платформенные библиотеки ESP-IDF :: их не нужно подключать в platformio.ini
#include "esp_log.h"

// Частные библиотеки проекта :: их не нужно подключать в platformio.ini
#include "project_lib_1.h"
#include "project_lib_2.h"

// Глобальные общие локальные библиотеки :: их не нужно подключать в platformio.ini
#include "global_lib_1.h"

// Общие локальные библиотеки :: подключенные в platformio.ini через lib_extra_dirs
#include "shared_lib_1.h"
#include "shared_lib_2.h"

// Общие локальные библиотеки :: подключенные в platformio.ini через lib_deps / symlink://
#include "symlink_lib_1.h"
#include "symlink_lib_2.h"

void app_main() 
{
  ESP_LOGE("app_main", "Message from the main function of the program");
  // Вызываем функции из частных библиотек проекта
  project_lib_1_function();
  project_lib_2_function();
  // Вызываем функции из глобальных общих библиотек
  global_lib_1_function();
  // Вызываем функции из общих локальных библиотек
  shared_lib_1_function();
  shared_lib_2_function();
  // Вызываем функции из общих локальных библиотек
  symlink_lib_1_function();
  symlink_lib_2_function();
}