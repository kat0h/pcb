#include "ch32fun.h"
#include "ch32v003_GPIO_branchless.h"
#include <stdio.h>
#include <string.h>
#define WS2812BSIMPLE_IMPLEMENTATION
#include "ws2812b_simple.h"
// USB
#include "rv003usb.h"
// neopixel
#include "rainbow.h"

// ピンの定義
#define BUTTON_PIN GPIOv_from_PORT_PIN(GPIO_port_A, 1)
#define EX_IO GPIOv_from_PORT_PIN(GPIO_port_C, 1)
#define NEOPIXEL_PIN 2 // PA2

// 前のループでボタンが押されていたかを判定
uint8_t before_btn_state = 0;

int main() {
  SystemInit();
  funGpioInitAll();
  GPIO_port_enable(GPIO_port_A);
  GPIO_port_enable(GPIO_port_C);
  GPIO_pinMode(BUTTON_PIN, GPIO_pinMode_I_pullDown, GPIO_Speed_In);
  GPIO_pinMode(EX_IO, GPIO_pinMode_O_pushPull, GPIO_Speed_2MHz);
  // 初期状態は3つとも消灯
  WS2812BSimpleSend(
      GPIOA, NEOPIXEL_PIN,
      (uint8_t[]){0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 9);
  Delay_Ms(1);
  usb_setup();
  before_btn_state = GPIO_digitalRead(BUTTON_PIN);
  while (1) {
    uint8_t current_btn_state = GPIO_digitalRead(BUTTON_PIN);
    if (current_btn_state == 1) {
      // LEDを光らせる
      uint8_t all_led_data[9];
      for (int i = 0; i < 3; i++)
        get_rainbow_for_led(i, 3, wheel_pos, &all_led_data[i * 3]);
      WS2812BSimpleSend(GPIOA, NEOPIXEL_PIN, all_led_data, 9);
      wheel_pos += 2;
      Delay_Ms(10);
    } else {
      WS2812BSimpleSend(
          GPIOA, NEOPIXEL_PIN,
          (uint8_t[]){0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 9);
      Delay_Ms(10);
    }
    before_btn_state = current_btn_state;
  }
}

void usb_handle_user_data(struct usb_endpoint *e, int current_endpoint,
                          uint8_t *data, int len,
                          struct rv003usb_internal *ist) {}

uint8_t tik = 0;
void usb_handle_user_in_request(struct usb_endpoint *e, uint8_t *scratchpad,
                                int endp, uint32_t sendtok,
                                struct rv003usb_internal *ist) {
  if (endp == 2) {
    // Keyboard (8 bytes)
    static uint8_t tsajoystick[8] = {0x00};
    usb_send_data(tsajoystick, 8, 0, sendtok);
    uint8_t btn = GPIO_digitalRead(BUTTON_PIN);
    // ボタンが押されていたらuecと入力する
    if (tik == 0 && btn == 1 && before_btn_state == 0) {
      tsajoystick[4] = 0x18;
      tik = 1;
    } else if (tik > 0) {
      if (tik == 1) {
        tsajoystick[4] = 0x08;
        tik += 1;
      } else if (tik == 2) {
        tsajoystick[4] = 0x06;
        tik = 0;
      }
    } else {
      tsajoystick[4] = 0;
    }
    before_btn_state = GPIO_digitalRead(BUTTON_PIN);
  } else {
    usb_send_empty(sendtok);
  }
}
