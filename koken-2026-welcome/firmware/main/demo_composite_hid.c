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

// 1. AWU用の割り込みハンドラを定義する (これがないと復帰時にフリーズします)
// __attribute__((interrupt)) はRISC-Vの割り込み処理に必須です
void AWU_IRQHandler(void) __attribute__((interrupt));
void AWU_IRQHandler(void) {
    // EXTI Line 9 (AWU) の割り込みフラグをクリア
    EXTI->INTFR = (1 << 9);
}

void awu_init_30ms(void) {
    // PWRモジュールのクロックとLSI(128kHz)を有効化
    RCC->APB1PCENR |= RCC_APB1Periph_PWR;
    RCC->RSTSCKR |= RCC_LSION;
    while ((RCC->RSTSCKR & RCC_LSIRDY) == 0);
    // 2. イベント(EVENR)ではなく【割り込み(INTENR)】を有効化
    EXTI->INTENR |= (1 << 9);
    // 3. AWUの仕様に合わせて【立ち下がりエッジ(FTENR)】に変更
    EXTI->FTENR |= (1 << 9);
    // PFIC (割り込みコントローラ) で AWU 割り込みを許可 (AWU_IRQn = 4)
    NVIC_EnableIRQ(AWU_IRQn);
    // 30msの設定: (59+1)*64 / 128000 = 0.03秒
    PWR->AWUPSC = 7;
    PWR->AWUWR  = 59;
    // AWUを有効化
    PWR->AWUCSR |= (1 << 1);
}

void enter_stop_mode_30ms(void) {
    // スリープに入る前に念のためフラグをクリア
    EXTI->INTFR = (1 << 9);
    // ストップモードを選択
    PWR->CTLR &= ~PWR_CTLR_PDDS;
    PFIC->SCTLR |= (1 << 2);
    // WFI (Wait For Interrupt) 命令でスリープに移行
    __asm__ volatile ("wfi");
    // --- 30ms経過後、AWU_IRQHandler が実行され、ここに戻ってくる ---
    // SLEEPDEEPを解除して通常状態に戻す
    PFIC->SCTLR &= ~(1 << 2);
}

void battery_main() {
  awu_init_30ms();
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
    enter_stop_mode_30ms();
  }
}

void keyboard_main() {
  battery_main();
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
