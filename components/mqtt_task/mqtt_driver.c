#include "mqtt_driver.h"

#include "esp_event.h"
#include "mqtt_client.h"

#include "common.h"
#include "event_manager.h"
#include "queue_types_manager.h"
#include "switch_task.h"

static esp_mqtt_client_handle_t client;
static uint8_t connection_retries = 0;

#if defined(CONFIG_MQTT_RELEASE) && !defined(CONFIG_MQTT_DEBUG)
#define MQTT_SERVERIP      CONFIG_MQTT_RELEASE_SERVER_IP
#define MQTT_CLIENTID      CONFIG_MQTT_RELEASE_CLIEND_ID
#define MQTT_PORT          CONFIG_MQTT_RELEASE_PORT
#define MQTT_RECON_TIMEOUT CONFIG_MQTT_RELEASE_RECON_TIMEOUT

#elif defined(CONFIG_MQTT_DEBUG) && !defined(CONFIG_MQTT_RELEASE)
#define MQTT_SERVERIP      CONFIG_MQTT_DEBUG_SERVER_IP
#define MQTT_CLIENTID      CONFIG_MQTT_DEBUG_CLIEND_ID
#define MQTT_PORT          CONFIG_MQTT_DEBUG_PORT
#define MQTT_RECON_TIMEOUT CONFIG_MQTT_DEBUG_RECON_TIMEOUT

#else
#error "Invalid MQTT mode selected"
#endif

#if CONFIG_BATTERY_LOGGER_EN
#include "logger.h"
DECLARE_LOG(MQTT DRIVER)
#else
#undef LOG
#define LOG_I(...)
#define LOG_W(...)
#define LOG_E(...)
#endif

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    switch (event_id) {
    case MQTT_EVENT_ERROR:
        LOG_I(TAG, "MQTT event error");
        break;
    case MQTT_EVENT_CONNECTED:
        set_event_bit(MQTT_ISR_GROUP, MQTT_CONNECTED);
        break;
    case MQTT_EVENT_DISCONNECTED:
        set_event_bit(MQTT_ISR_GROUP, MQTT_DISCONNECTED);
        break;
    case MQTT_EVENT_PUBLISHED:
        LOG_I(TAG, "Message published");
        break;
    case MQTT_EVENT_BEFORE_CONNECT:
        LOG_I(TAG, "Event before connect");
        break;
    default:
        LOG_W(TAG, "Unknown event IP. ID: %ld", event_id);
        break;
    }
}

esp_err_t mqtt_init() {
    LOG_I(TAG, "init");
    connection_retries = 0;

    char MQTT_URI[] = "mqtt://" MQTT_SERVERIP ":" MQTT_PORT;
    LOG_I(TAG, "%s", MQTT_URI);

    esp_mqtt_client_config_t config = {
      .broker.address.uri           = MQTT_URI,
      .credentials.client_id        = MQTT_CLIENTID,
      .network.reconnect_timeout_ms = MQTT_RECON_TIMEOUT,
    };

    client = esp_mqtt_client_init(&config);

    if (!client) {
        LOG_E(TAG, "Failed to initialize client");
        return ESP_FAIL;
    }

    ESP_ERROR_CHECK(esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL));

    ESP_ERROR_CHECK(esp_mqtt_client_start(client));

    LOG_I(TAG, "Client has been initiated");

    return ESP_OK;
}

esp_err_t mqtt_publish(const char *topic, const char *msg, uint8_t size, uint8_t qos) {
    return (esp_mqtt_client_publish(client, topic, msg, size, qos, 0) == -1) ? ESP_FAIL : ESP_OK;
}

void mqtt_run_process() {
    EventBits_t mqtt_event_bits = wait_event_bit(MQTT_ISR_GROUP, MQTT_ALL, pdTRUE, pdFALSE, WAIT_UNTILL_BIT);

    if (mqtt_event_bits == MQTT_CONNECTED) {
        connection_retries = 0;
        LOG_I(TAG, "Connected");
        clear_event_bits(MQTT_STATUS_GROUP, MQTT_DISCONNECTED);
        set_event_bit(MQTT_STATUS_GROUP, MQTT_CONNECTED);
        semphr_give(SEMPHR_MQTT_CON);
    }

    if (mqtt_event_bits == MQTT_DISCONNECTED) {
        ++connection_retries;
        if (get_event_bits(WIFI_STATUS_EVENT_GROUP) == WIFI_DISCONNECTED) {
            wait_event_bit(WIFI_STATUS_EVENT_GROUP, WIFI_CONNECTED, pdFALSE, pdFALSE, WAIT_UNTILL_BIT);
        }
        LOG_W(TAG, "Disconnected, trying %d time", connection_retries);
        clear_event_bits(MQTT_STATUS_GROUP, MQTT_CONNECTED);
        set_event_bit(MQTT_STATUS_GROUP, MQTT_DISCONNECTED);
        semphr_give(SEMPHR_MQTT_DISCON);
    }
}
