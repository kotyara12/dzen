#include <stddef.h>
#include <stdlib.h>
#include <time.h>
#include "esp_sntp.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

const char* logTAG = "time";

void sntp_notification(struct timeval *tv)
{
  struct tm timeinfo;
  char strftime_buf[20];

  localtime_r(&tv->tv_sec, &timeinfo);
  if (timeinfo.tm_year < (1970 - 1900)) {
    ESP_LOGE(logTAG, "Time synchronization failed!");
  } else {
    // Post time event
    // eventLoopPost(RE_TIME_EVENTS, RE_TIME_SNTP_SYNC_OK, nullptr, 0, portMAX_DELAY);
    // Log
    strftime(strftime_buf, sizeof(strftime_buf), "%d.%m.%Y %H:%M:%S", &timeinfo);
    ESP_LOGI(logTAG, "Time synchronization completed, current time: %s", strftime_buf);
  };
}

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

  // Получаем время с точностью до миллисекунды
  struct timeval tv_now;
  gettimeofday(&tv_now, NULL);
  int64_t time_us = (int64_t)tv_now.tv_sec * 1000000L + (int64_t)tv_now.tv_usec;

  // Получаем количество микросекунд с момента запуска процессора
  uint64_t usec = esp_timer_get_time();
  // Получаем количество выполненных циклов ЦП
  uint32_t cycles1 = esp_cpu_get_cycle_count();
  uint32_t cycles2 = cpu_hal_get_cycle_count();
  // Получаем количество выполненных тиков FreeRTOS 
  xTaskGetTickCount();

  struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    return spec.tv_nsec / 1000000 + spec.tv_sec * 1000;

  // Установка часового пояска
  setenv("TZ", "MSK-3", 1);
  tzset();

  // Запускаем синхронизацию времени с SNTP
  sntp_setoperatingmode(SNTP_OPMODE_POLL);
  sntp_set_time_sync_notification_cb(sntp_notification);
  sntp_setservername(0, "pool.ntp.org");
  sntp_setservername(1, "time.nist.gov");
  sntp_init();

  // Заново получаем время после обновления времени
  now1 = time(NULL);

  // Конвертация целочисленного UNIX-формата в структуру с отдельными полями
  struct tm tm_now;
  localtime_r(&now1, &tm_now);
  ESP_LOGI(logTAG, "hour = %.2d, min = %.2d, sec = %.2d, week day = %d, month day = %d, year day = %d, month = %d, year = %d, is dst = %d", 
    tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec, 
    tm_now.tm_wday, tm_now.tm_mday, tm_now.tm_yday, 
    tm_now.tm_mon, tm_now.tm_year, 
    tm_now.tm_isdst);

  // Форматирование строк с датой и временем
  char strftime_buf[64];
  strftime(strftime_buf, sizeof(strftime_buf), "%Y-%m-%d %H:%M:%S", &tm_now);
}