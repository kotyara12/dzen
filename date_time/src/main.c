#include <stddef.h>
#include <time.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

const char* logTAG = "time";

void app_main() 
{
  vTaskDelay(1000);

  // Получаем время, способ 1
  time_t now1 = time(NULL);
  ESP_LOGI(logTAG, "now1 = %d", (uint32_t)now1);

  vTaskDelay(1000);

  // Получаем время, способ 2
  time_t now2;
  time(&now2);
  ESP_LOGI(logTAG, "now2 = %d", (uint32_t)now2);

  // Конвертация целочисленного UNIX-формата в структуру с отдельными полями
  struct tm tm_now;
  localtime_r(&now1, &tm_now);
  ESP_LOGI(logTAG, "hour = %.2d, min = %.2d, sec = %.2d, week day = %d, month day = %d, year day = %d, month = %d, year = %d, is dst = %d", 
    tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec, 
    tm_now.tm_wday, tm_now.tm_mday, tm_now.tm_yday, 
    tm_now.tm_mon, tm_now.tm_year, 
    tm_now.tm_isdst);
}