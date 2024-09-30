#include "led_handler.h"

#include "driver/gpio.h"
#include "driver/rmt_encoder.h"
#include "driver/rmt_tx.h"
#include "esp_heap_caps.h"

#include "common.h"
#include "event_manager.h"

#define LED_STRIP_GPIO    CONFIG_LED_STRIP_PIN
#define LED_RESOLUTION_HZ (10000000)

static uint8_t led_strip_pixels[COMMON_NUMBER_OF_SWITCHES * 3];
static uint16_t led_strip_pixels_len;

typedef struct {
    rmt_encoder_t base;
    rmt_encoder_t *bytes_encoder;
    rmt_encoder_t *copy_encoder;
    int state;
    rmt_symbol_word_t reset_code;
} rmt_led_encoder_t;

struct led_handler_context {
    gpio_num_t gpio_num;
    rmt_channel_handle_t rmt_channel;
    rmt_encoder_handle_t encoder;
};

static struct led_handler_context ctx = {};

static esp_err_t led_rmt_driver_init_rmt() {
    rmt_tx_channel_config_t tx_chan_config = {
      .clk_src           = RMT_CLK_SRC_DEFAULT,
      .gpio_num          = ctx.gpio_num,
      .mem_block_symbols = 64,
      .resolution_hz     = LED_RESOLUTION_HZ,
      .trans_queue_depth = 4,
    };

    return rmt_new_tx_channel(&tx_chan_config, &ctx.rmt_channel);
}

static size_t led_rmt_driver_encode(rmt_encoder_t *encoder,
                                    rmt_channel_handle_t channel,
                                    const void *primary_data,
                                    size_t data_size,
                                    rmt_encode_state_t *ret_state) {
    rmt_led_encoder_t *led_encoder     = __containerof(encoder, rmt_led_encoder_t, base);
    rmt_encoder_handle_t bytes_encoder = led_encoder->bytes_encoder;
    rmt_encoder_handle_t copy_encoder  = led_encoder->copy_encoder;
    rmt_encode_state_t session_state   = RMT_ENCODING_RESET;
    rmt_encode_state_t state           = RMT_ENCODING_RESET;
    size_t encoded_symbols             = 0;

    if (!led_encoder->state) {
        encoded_symbols += bytes_encoder->encode(bytes_encoder, channel, primary_data, data_size, &session_state);
        if (session_state & RMT_ENCODING_COMPLETE) {
            led_encoder->state = 1;
        }
        if (session_state & RMT_ENCODING_MEM_FULL) {
            state |= RMT_ENCODING_MEM_FULL;
            *ret_state = state;
            return encoded_symbols;
        }
    }

    encoded_symbols += copy_encoder->encode(
      copy_encoder, channel, &led_encoder->reset_code, sizeof(led_encoder->reset_code), &session_state);
    if (session_state & RMT_ENCODING_COMPLETE) {
        led_encoder->state = RMT_ENCODING_RESET;
        state |= RMT_ENCODING_COMPLETE;
    }
    if (session_state & RMT_ENCODING_MEM_FULL) {
        state |= RMT_ENCODING_MEM_FULL;
    }

    *ret_state = state;
    return encoded_symbols;
}

static esp_err_t led_rmt_driver_delete_encoder(rmt_encoder_t *encoder) {
    rmt_led_encoder_t *led_encoder = __containerof(encoder, rmt_led_encoder_t, base);
    rmt_del_encoder(led_encoder->bytes_encoder);
    rmt_del_encoder(led_encoder->copy_encoder);
    heap_caps_free(led_encoder);
    return ESP_OK;
}

static esp_err_t led_rmt_driver_reset_encoder(rmt_encoder_t *encoder) {
    rmt_led_encoder_t *led_encoder = __containerof(encoder, rmt_led_encoder_t, base);
    rmt_encoder_reset(led_encoder->bytes_encoder);
    rmt_encoder_reset(led_encoder->copy_encoder);
    led_encoder->state = RMT_ENCODING_RESET;
    return ESP_OK;
}

static esp_err_t led_rmt_driver_create_encoder() {
    rmt_led_encoder_t *led_encoder = NULL;
    led_encoder                    = heap_caps_calloc(1, sizeof(rmt_led_encoder_t), MALLOC_CAP_DEFAULT);
    led_encoder->base.encode       = led_rmt_driver_encode;
    led_encoder->base.del          = led_rmt_driver_delete_encoder;
    led_encoder->base.reset        = led_rmt_driver_reset_encoder;
    rmt_bytes_encoder_config_t bytes_encoder_config = {
        .bit0 = {
            .level0 = 1,
            .duration0 = 0.3 * LED_RESOLUTION_HZ / 1000000,
            .level1 = 0,
            .duration1 = 0.9 * LED_RESOLUTION_HZ / 1000000,
        },
        .bit1 = {
            .level0 = 1,
            .duration0 = 0.9 * LED_RESOLUTION_HZ / 1000000,
            .level1 = 0,
            .duration1 = 0.3 * LED_RESOLUTION_HZ / 1000000,
        },
        .flags.msb_first = 1
    };
    rmt_new_bytes_encoder(&bytes_encoder_config, &led_encoder->bytes_encoder);
    rmt_copy_encoder_config_t copy_encoder_config = {};
    rmt_new_copy_encoder(&copy_encoder_config, &led_encoder->copy_encoder);

    uint32_t reset_ticks    = LED_RESOLUTION_HZ / 1000000 * 50 / 2;
    led_encoder->reset_code = (rmt_symbol_word_t){
      .level0    = 0,
      .duration0 = reset_ticks,
      .level1    = 0,
      .duration1 = reset_ticks,
    };
    ctx.encoder = &led_encoder->base;

    return ESP_OK;
}

esp_err_t led_rmt_driver_init() {
    ctx.gpio_num         = LED_STRIP_GPIO;
    led_strip_pixels_len = sizeof(led_strip_pixels);

    led_rmt_driver_init_rmt();
    led_rmt_driver_create_encoder();
    rmt_enable(ctx.rmt_channel);
    led_handler_clear_all();

    return ESP_OK;
}

esp_err_t led_rmt_driver_send() {
    static const rmt_transmit_config_t tx_config = {
      .loop_count = 0,
    };
    return rmt_transmit(ctx.rmt_channel, ctx.encoder, &led_strip_pixels, led_strip_pixels_len, &tx_config);
}

esp_err_t led_handler_set_color(uint8_t red, uint8_t green, uint8_t blue, led_bit_t led_bit) {
    led_strip_pixels[led_bit * 3 + 0] = red;
    led_strip_pixels[led_bit * 3 + 1] = green;
    led_strip_pixels[led_bit * 3 + 2] = blue;

    return led_rmt_driver_send();
}

esp_err_t led_handler_clear_all() {
    for (uint8_t led = 0; led < COMMON_NUMBER_OF_SWITCHES; led++) {
        led_handler_set_color(0, 0, 0, (led_bit_t)led);
    }
    return ESP_OK;
}
