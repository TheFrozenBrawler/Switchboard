#ifndef COMPONENTS_LOGGER_LOGGER_H_
#define COMPONENTS_LOGGER_LOGGER_H_

#if defined(CONFIG_ENV_DEBUG) && defined(CONFIG_ENV_RELEASE)
#error "Only one environment can be selected simultaneously"
#endif

#if defined(CONFIG_ENV_DEBUG)
#include <esp_log.h>

#define DECLARE_LOG(X) static const char *const TAG = #X;

#define LOG_I(TAG, ...) ESP_LOGI(TAG, __VA_ARGS__)
#define LOG_W(TAG, ...) ESP_LOGW(TAG, __VA_ARGS__)
#define LOG_E(TAG, ...) ESP_LOGE(TAG, __VA_ARGS__)

#elif defined(CONFIG_ENV_RELEASE)

#define DECLARE_LOG(X)
#define LOG_I(TAG, ...)
#define LOG_W(TAG, ...)
#define LOG_E(TAG, ...)

#endif

#endif  // COMPONENTS_LOGGER_LOGGER_H_
