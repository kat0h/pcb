#include "ch32fun.h"
#include "ch32v003_GPIO_branchless.h"
#include <stdio.h>
#include <string.h>
// USB
#include "rv003usb.h"
// neopixel
#include "../lib/ch32fun/examples/ws2812bdemo/color_utilities.h"
#define WS2812BSIMPLE_IMPLEMENTATION
#include "ws2812b_simple.h"
// adc
#include "getpowervoltage.h"

// ピンの定義
#define BUTTON_PIN GPIOv_from_PORT_PIN(GPIO_port_A, 1)
#define EX_IO GPIOv_from_PORT_PIN(GPIO_port_C, 1)
#define NEOPIXEL_PIN 2 // PA2

// 前のループでボタンが押されていたか
uint8_t before_btn_state = 0;

void set_cpu_prescaler(uint32_t rcc_cfgr0_hpre) {
    // RCC_CFGR0 レジスタの HPRE (AHB Prescaler) ビットをクリアして設定
    uint32_t temp = RCC->CFGR0;
    temp &= ~(RCC_HPRE); // マスク（既存の設定をクリア）
    temp |= rcc_cfgr0_hpre;
    RCC->CFGR0 = temp;
}


int main() {
  SystemInit();
  funGpioInitAll();
  // ボタンとNeopixelの初期化
  GPIO_port_enable(GPIO_port_A);
  GPIO_pinMode(BUTTON_PIN, GPIO_pinMode_I_pullDown, GPIO_Speed_In);
  WS2812BSimpleSend(
      GPIOA, NEOPIXEL_PIN,
      (uint8_t[]){0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 9);
  // 電源を判別
  adc_init_vref();
  uint8_t usb_connected = get_vcc_mv() > 3100;
  // usbの初期化
  if (usb_connected)
    usb_setup();
  else {
    // USB接続でない場合は動作クロックを落とす
    // set_cpu_prescaler(RCC_HPRE_DIV8);
    // TODO:
  }
  before_btn_state = GPIO_digitalRead(BUTTON_PIN);
  // 色相環のシフト量
  uint8_t wheel_pos = 0;
  uint8_t brightness = usb_connected ? 50 : 5; // 電源電圧によって明るさを変更
  uint8_t all_led_data[9] = {0};
  // メインループ
  while (1) {
    uint8_t current_btn_state = GPIO_digitalRead(BUTTON_PIN);
    wheel_pos += 4;
    if (current_btn_state == 1 || !usb_connected) {
      for (int i = 0; i < 3; i++) {
        // 各LEDの色相をずらす
        uint8_t hue = wheel_pos + (i * (255 / 4));
        uint32_t color = EHSVtoHEX(hue, 255, brightness);
        all_led_data[i * 3 + 0] = (color >> 8) & 0xFF;
        all_led_data[i * 3 + 1] = (color >> 0) & 0xFF;
        all_led_data[i * 3 + 2] = (color >> 16) & 0xFF;
      }
      WS2812BSimpleSend(GPIOA, NEOPIXEL_PIN, all_led_data, 9);
    } else {
      WS2812BSimpleSend(
          GPIOA, NEOPIXEL_PIN,
          (uint8_t[]){0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 9);
    }
    Delay_Ms(30);
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
