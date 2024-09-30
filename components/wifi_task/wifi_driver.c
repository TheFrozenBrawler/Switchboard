#include "wifi_driver.h"

#include "esp_wifi.h"
#include "lwip/inet.h"
#include "string.h"

#include "common.h"
#include "event_manager.h"
#include "ntp_driver.h"

#if CONFIG_WIFI_LOGGER_EN
#include "logger.h"
DECLARE_LOG(WIFI MANAGER)
#else
#undef LOG
#define LOG_I(...)
#define LOG_W(...)
#define LOG_E(...)
#endif

#if defined(CONFIG_WIFI_RELEASE) && !defined(CONFIG_WIFI_DEBUG)

#define WIFI_SSID        CONFIG_WIFI_RELEASE_SSID
#define WIFI_PASS        CONFIG_WIFI_RELEASE_PASSWORD
#define WIFI_GATEWAY     CONFIG_WIFI_RELEASE_GATEWAY
#define WIFI_SUBNET_MASK CONFIG_WIFI_RELEASE_SUBNET_MASK
#define WIFI_STATIC_IP   CONFIG_WIFI_RELEASE_IP

#elif defined(CONFIG_WIFI_DEBUG) && !defined(CONFIG_WIFI_RELEASE)

#define WIFI_SSID        CONFIG_WIFI_DEBUG_SSID
#define WIFI_PASS        CONFIG_WIFI_DEBUG_PASSWORD
#define WIFI_GATEWAY     CONFIG_WIFI_DEBUG_GATEWAY
#define WIFI_SUBNET_MASK CONFIG_WIFI_DEBUG_SUBNET_MASK
#define WIFI_STATIC_IP   CONFIG_WIFI_DEBUG_IP

#else
#error "Invalid WiFi mode selected"
#endif

struct wifi_manager_context {
    uint16_t connection_retries;
    esp_netif_t *esp_netif_sta;
};

static struct wifi_manager_context ctx = {};

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    switch (event_id) {
    case WIFI_EVENT_STA_START: {
        set_event_bit(WIFI_ISR_EVENT_GROUP, WIFI_STARTED);
        return;
    }
    case WIFI_EVENT_STA_STOP: {
        set_event_bit(WIFI_ISR_EVENT_GROUP, WIFI_IDLE);
        return;
    }
    case WIFI_EVENT_STA_DISCONNECTED: {
        set_event_bit(WIFI_ISR_EVENT_GROUP, WIFI_DISCONNECTED);
        return;
    }
    case WIFI_EVENT_STA_CONNECTED: {
        set_event_bit(WIFI_ISR_EVENT_GROUP, WIFI_CONNECTED);
        return;
    }
    case WIFI_EVENT_STA_BEACON_TIMEOUT: {
        LOG_W(TAG, "Beacon timeout. Connection has been broken.");
        return;
    }
    default:
        LOG_W(TAG, "Unknown event. ID: %ld", event_id);
        return;
    }
}

static void ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    switch (event_id) {
    case IP_EVENT_STA_GOT_IP:
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        clear_event_bits(IP_STATUS_EVENT_GROUP, IP_LOST);
        set_event_bit(IP_STATUS_EVENT_GROUP, IP_GOT);
        LOG_I(TAG, "WiFi init process ended, aquired ip addres: " IPSTR, IP2STR(&event->ip_info.ip));
        return;

    case IP_EVENT_STA_LOST_IP:
        clear_event_bits(IP_STATUS_EVENT_GROUP, IP_GOT);
        set_event_bit(IP_STATUS_EVENT_GROUP, IP_LOST);
        LOG_W(TAG, "Lost IP");
        return;

    default:
        LOG_W(TAG, "Unknown event IP. ID: %ld", event_id);
        return;
    }
}

static esp_err_t wifi_manager_set_dns_server() {
    esp_netif_dns_info_t dns;
    dns.ip.u_addr.ip4.addr = ipaddr_addr(WIFI_GATEWAY);
    dns.ip.type            = ESP_IPADDR_TYPE_V4;

    LOG_I(TAG, "DNS: %s", WIFI_GATEWAY);

    esp_netif_set_dns_info(ctx.esp_netif_sta, ESP_NETIF_DNS_MAIN, &dns);
    return ESP_OK;
}

static esp_err_t wifi_manager_set_static_ip() {
    esp_netif_dhcpc_stop(ctx.esp_netif_sta);
    esp_netif_ip_info_t ip;
    memset(&ip, 0, sizeof(esp_netif_ip_info_t));
    ip.ip.addr      = ipaddr_addr(WIFI_STATIC_IP);
    ip.netmask.addr = ipaddr_addr(WIFI_SUBNET_MASK);
    ip.gw.addr      = ipaddr_addr(WIFI_GATEWAY);

    LOG_I(TAG, "setting static ip: %s, netmask: %s, gw: %s", WIFI_STATIC_IP, WIFI_SUBNET_MASK, WIFI_GATEWAY);

    esp_netif_set_ip_info(ctx.esp_netif_sta, &ip);
    wifi_manager_set_dns_server();

    return ESP_OK;
}

esp_err_t wifi_init(void) {
    LOG_I(TAG, "Wifi init start");
    ctx.connection_retries = 0;

    RET_AND_LOG_ON_ERR(esp_netif_init(), "Esp netif init");
    RET_AND_LOG_ON_ERR(esp_event_loop_create_default(), "esp event loop create");
    ctx.esp_netif_sta = esp_netif_create_default_wifi_sta();

    wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
    RET_AND_LOG_ON_ERR(esp_wifi_init(&init_config), "esp wifi init");
    RET_AND_LOG_ON_ERR(
      esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL, NULL),
      "wifi event hadler register");

    RET_AND_LOG_ON_ERR(esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, ip_event_handler, NULL, NULL),
                       "ip event hadler register");

    wifi_config_t wifi_config = {
        .sta = {
            .ssid               = WIFI_SSID,
            .password           = WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg            = {
                .capable  = true,
                .required = false,
            },
        },
    };

    RET_AND_LOG_ON_ERR(esp_wifi_set_mode(WIFI_MODE_STA), "wifi set mode");
    RET_AND_LOG_ON_ERR(esp_wifi_set_config(WIFI_IF_STA, &wifi_config), "wifi set config");
    RET_AND_LOG_ON_ERR(esp_wifi_start(), "wifi start");

    RET_AND_LOG_ON_ERR(ntp_driver_init(), "NTP driver init");

    LOG_I(TAG, "WiFi driver initiated ");
    return ESP_OK;
}

void wifi_run_process() {
    EventBits_t wifi_event_bits = wait_event_bit(WIFI_ISR_EVENT_GROUP, WIFI_ALL, pdTRUE, pdFALSE, WAIT_UNTILL_BIT);

    if (wifi_event_bits == WIFI_STARTED) {
        ctx.connection_retries = 0;
        LOG_I(TAG, "Started WiFi connection, connecting...");
        esp_wifi_connect();
        clear_event_bits(WIFI_RUNNING_EVENT_GROUP, WIFI_STOPPED);
        set_event_bit(WIFI_RUNNING_EVENT_GROUP, WIFI_RUNNING);
    }

    if (wifi_event_bits == WIFI_IDLE) {
        LOG_I(TAG, "stopped");
        clear_event_bits(WIFI_STATUS_EVENT_GROUP, WIFI_RUNNING);
        set_event_bit(WIFI_STATUS_EVENT_GROUP, WIFI_STOPPED);
    }

    if (wifi_event_bits == WIFI_DISCONNECTED) {
        ++ctx.connection_retries;
        esp_wifi_connect();
        LOG_W(TAG, "disconnected, trying %d time", ctx.connection_retries);
        clear_event_bits(MQTT_STATUS_GROUP, MQTT_CONNECTED);
        set_event_bit(MQTT_STATUS_GROUP, MQTT_DISCONNECTED);
        clear_event_bits(WIFI_STATUS_EVENT_GROUP, WIFI_CONNECTED);
        set_event_bit(WIFI_STATUS_EVENT_GROUP, WIFI_DISCONNECTED);
        ntp_driver_stop();
    }

    if (wifi_event_bits == WIFI_CONNECTED) {
        ntp_driver_start();
        if (wifi_manager_set_static_ip() != ESP_OK) {
            LOG_I(TAG, "setting static IP fail");
        }
        LOG_I(TAG, "connected");
        clear_event_bits(WIFI_STATUS_EVENT_GROUP, WIFI_DISCONNECTED);
        set_event_bit(WIFI_STATUS_EVENT_GROUP, WIFI_CONNECTED);
    }
}
