#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "mqtt_client.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"

#define WIFI_SSID       "wifiname" // wifi name
#define WIFI_PASS       "password" // password 
    #define MQTT_BROKER_URI "mqtt://192.168.xxx.xx" // put ur hostname here 
#define GPIO_DIGITAL1 18
#define GPIO_DIGITAL2 19
#define GPIO_DIGITAL3 34
#define GPIO_DIGITAL4 35

#define ADC_CHANNEL1 ADC_CHANNEL_0  // GPIO36
#define ADC_CHANNEL2 ADC_CHANNEL_3  // GPIO39

static const char *TAG = "MQTT_APP";

static esp_mqtt_client_handle_t client = NULL;
static adc_oneshot_unit_handle_t adc1_handle;

// ================= MQTT Publish Task =================
void read_and_publish_task(void *arg) {
    while (1) {
        int d1 = gpio_get_level(GPIO_DIGITAL1);
        int d2 = gpio_get_level(GPIO_DIGITAL2);
        int d3 = gpio_get_level(GPIO_DIGITAL3);
        int d4 = gpio_get_level(GPIO_DIGITAL4);

        int adc_raw1 = 0, adc_raw2 = 0;
        adc_oneshot_read(adc1_handle, ADC_CHANNEL1, &adc_raw1);
        adc_oneshot_read(adc1_handle, ADC_CHANNEL2, &adc_raw2);

        char msg[32];

        snprintf(msg, sizeof(msg), "{\"value\": %d}", d1);
        esp_mqtt_client_publish(client, "/esp32/digital1", msg, 0, 1, 0);
        snprintf(msg, sizeof(msg), "{\"value\": %d}", d2);
        esp_mqtt_client_publish(client, "/esp32/digital2", msg, 0, 1, 0);
        snprintf(msg, sizeof(msg), "{\"value\": %d}", d3);
        esp_mqtt_client_publish(client, "/esp32/digital3", msg, 0, 1, 0);
        snprintf(msg, sizeof(msg), "{\"value\": %d}", d4);
        esp_mqtt_client_publish(client, "/esp32/digital4", msg, 0, 1, 0);
        snprintf(msg, sizeof(msg), "{\"value\": %d}", adc_raw1);
        esp_mqtt_client_publish(client, "/esp32/analog1", msg, 0, 1, 0);
        snprintf(msg, sizeof(msg), "{\"value\": %d}", adc_raw2);
        esp_mqtt_client_publish(client, "/esp32/analog2", msg, 0, 1, 0);

        ESP_LOGI(TAG, "Published: d1=%d d2=%d d3=%d d4=%d a1=%d a2=%d", d1, d2, d3, d4, adc_raw1, adc_raw2);

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

// ================= MQTT Event Handler =================
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT connected");
            esp_mqtt_client_publish(client, "/esp32/test", "hi from esp32", 0, 1, 0);
            xTaskCreate(read_and_publish_task, "read_and_publish_task", 4096, NULL, 5, NULL);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, " MQTT disconnected");
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, " MQTT error");
            break;
        default:
            break;
    }
}

// ================= MQTT Start =================
void mqtt_app_start(void) {
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address = {
                .uri = MQTT_BROKER_URI,
            },
        },
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

// ================= Wi-Fi Event Handler =================
void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW("WiFi", "Disconnected. Retrying...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI("WiFi", "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        mqtt_app_start();  
    }
}

// ================= Wi-Fi Init =================
void wifi_init_sta(void) {
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
    esp_wifi_start();
}

// ================= App Main =================
void app_main(void) {
    ESP_ERROR_CHECK(nvs_flash_init());
    esp_log_level_set("wifi", ESP_LOG_DEBUG);
    gpio_set_direction(GPIO_DIGITAL1, GPIO_MODE_INPUT);
    gpio_set_direction(GPIO_DIGITAL2, GPIO_MODE_INPUT);
    gpio_set_direction(GPIO_DIGITAL3, GPIO_MODE_INPUT);
    gpio_set_direction(GPIO_DIGITAL4, GPIO_MODE_INPUT);

    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = ADC_UNIT_1
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &adc1_handle));

    adc_oneshot_chan_cfg_t chan_cfg = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL1, &chan_cfg));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL2, &chan_cfg));

    wifi_init_sta();
}
