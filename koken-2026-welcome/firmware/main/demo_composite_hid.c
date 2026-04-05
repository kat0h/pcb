#include "ch32fun.h"
#include "ch32v003_GPIO_branchless.h"
#include <stdio.h>
#include <string.h>
#include "rv003usb.h"
#include "../lib/ch32fun/examples/ws2812bdemo/color_utilities.h"
#define WS2812BSIMPLE_IMPLEMENTATION
#include "ws2812b_simple.h"
#include "getpowervoltage.h"

#define BT_PIN GPIOv_from_PORT_PIN(GPIO_port_A, 1)
#define NP_PIN 2 // PA2

void battery_main() {
  uint8_t wheel_pos = 0;
  uint8_t all_led_data[9] = {0};
  while (1) {
    uint8_t brightness = !GPIO_digitalRead(BT_PIN) ? 50 : 5;
    wheel_pos += 4;
    for (int i = 0; i < 3; i++) {
      // 各LEDの色相をずらす
      uint8_t hue = wheel_pos + (i * (255 / 4));
      uint32_t color = EHSVtoHEX(hue, 255, brightness);
      all_led_data[i * 3 + 0] = (color >> 8) & 0xFF;
      all_led_data[i * 3 + 1] = (color >> 0) & 0xFF;
      all_led_data[i * 3 + 2] = (color >> 16) & 0xFF;
    }
    WS2812BSimpleSend(GPIOA, NP_PIN, all_led_data, 9);
    Delay_Ms(30);
  }
}

void keyboard_main() {
  usb_setup();
}

void usb_handle_user_data(struct usb_endpoint *e, int current_endpoint,
                          uint8_t *data, int len,
                          struct rv003usb_internal *ist) {}
void usb_handle_user_in_request(struct usb_endpoint *e, uint8_t *scratchpad,
                                int endp, uint32_t sendtok,
                                struct rv003usb_internal *ist) {
  usb_send_empty(sendtok);
}

int main() {
  SystemInit();
  funGpioInitAll();
  GPIO_port_enable(GPIO_port_A); // initialize neopixel
  WS2812BSimpleSend(GPIOA, NP_PIN, (uint8_t[]){0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 6);
  GPIO_pinMode(BT_PIN, GPIO_pinMode_I_pullUp, GPIO_Speed_In); // initialize button
  adc_init_vref();
  uint8_t usb_connected = get_vcc_mv() > 3200; // power supply is usb?
  if (usb_connected) keyboard_main();
  else battery_main();
}
