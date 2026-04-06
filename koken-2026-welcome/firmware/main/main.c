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

#include "battery.c"
#include "keyboard.c"

int main() {
  SystemInit();
  funGpioInitAll();
  GPIO_port_enable(GPIO_port_A); // initialize neopixel
  WS2812BSimpleSend(GPIOA, NP_PIN, (uint8_t[]){0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 6);
  GPIO_pinMode(BT_PIN, GPIO_pinMode_I_pullUp, GPIO_Speed_In); // initialize button
  adc_init_vref();
  Delay_Ms(150);
  uint8_t usb_connected = get_vcc_mv() > 3150; // power supply is usb?
  if (usb_connected) keyboard_main();
  else battery_main();
}
