#include "ch32fun.h"
#include <stdio.h>
#include <string.h>
#include "rv003usb.h"
#define WS2812BSIMPLE_IMPLEMENTATION
#include "ch32v003_GPIO_branchless.h"
#include "ws2812b_simple.h"

extern struct rv003usb_internal rv003usb_internal_data;
#define BUTTON_PIN GPIOv_from_PORT_PIN(GPIO_port_A, 1)
#define EX_IO GPIOv_from_PORT_PIN(GPIO_port_C, 1)
#define NEOPIXEL_PIN 2 // PA2
#define BRIGHTNESS 5

uint8_t before_btn_state = 0;

// 虹色計算用の変数
uint8_t wheel_pos = 0;

// HSVからRGB（正確にはWS2812B用のGRB）に変換する簡易関数
// led_index: 現在設定するLEDの番号 (0, 1, 2)
// total_leds: 全体のLED数
// pos: 色相の基準値 (0-255)
void get_rainbow_for_led(uint8_t led_index, uint8_t total_leds, uint8_t base_pos, uint8_t *grb) {
    // 各LEDで色相をずらす
    // base_posからのオフセットを計算
    uint8_t pos = (base_pos + (led_index * (255 / total_leds))) % 255; 
    
    pos = 255 - pos; // 色相の方向を調整
    if(pos < 85) {
        grb[0] = 255 - pos * 3; grb[1] = 0; grb[2] = pos * 3; // G, R, B
    } else if(pos < 170) {
        pos -= 85;
        grb[0] = 0; grb[1] = pos * 3; grb[2] = 255 - pos * 3;
    } else {
        pos -= 170;
        grb[0] = pos * 3; grb[1] = 255 - pos * 3; grb[2] = 0;
    }
    grb[0] = (grb[0] * BRIGHTNESS) >> 8; // Green
    grb[1] = (grb[1] * BRIGHTNESS) >> 8; // Red
    grb[2] = (grb[2] * BRIGHTNESS) >> 8; // Blue
}

void set_cpu_prescaler(uint32_t rcc_cfgr0_hpre) {
    // RCC_CFGR0 レジスタの HPRE (AHB Prescaler) ビットをクリアして設定
    uint32_t temp = RCC->CFGR0;
    temp &= ~(RCC_HPRE); // マスク（既存の設定をクリア）
    temp |= rcc_cfgr0_hpre;
    RCC->CFGR0 = temp;
}

int main()
{
    SystemInit();
    funGpioInitAll();
    GPIO_port_enable(GPIO_port_A);
    GPIO_port_enable(GPIO_port_C);
  set_cpu_prescaler(RCC_HPRE_DIV1);
  set_cpu_prescaler(RCC_HPRE_DIV8);
    
    GPIO_pinMode(BUTTON_PIN, GPIO_pinMode_I_pullDown, GPIO_Speed_In);
    GPIO_pinMode(EX_IO, GPIO_pinMode_O_pushPull, GPIO_Speed_2MHz);
    
    // 初期状態は3つとも消灯
    WS2812BSimpleSend(GPIOA, NEOPIXEL_PIN, (uint8_t[]){0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 9);

    Delay_Ms(1); 
    // usb_setup();
    RCC->CTLR|=RCC_PLLON;
    RCC->CFGR0&=~0b11;
    while (1) {
      GPIO_digitalWrite(EX_IO, high);
      GPIO_digitalWrite(EX_IO, low);
    }

    before_btn_state = GPIO_digitalRead(BUTTON_PIN);

    Delay_Ms(1000);   
    while(1)
    {
        // USB処理
#if RV003USB_EVENT_DEBUGGING
        uint32_t * ue = GetUEvent();
        if( ue ) printf( "%lu %lx %lx %lx\n", ue[0], ue[1], ue[2], ue[3] );
#endif

        uint8_t current_btn_state = GPIO_digitalRead(BUTTON_PIN);
        
        if (current_btn_state == 1 || rv003usb_internal_data.my_address == 0) {
            // --- ボタンが押されている間：3つのLEDで虹色を回す ---
            uint8_t all_led_data[9]; // 3つのLED * 3バイト = 9バイト

            // 各LEDの色を計算してall_led_dataに格納
            for(int i = 0; i < 3; i++) {
                get_rainbow_for_led(i, 3, wheel_pos, &all_led_data[i * 3]);
            }

            WS2812BSimpleSend(GPIOA, NEOPIXEL_PIN, all_led_data, 9);
            
            wheel_pos += 2; // 数値を大きくすると色が早く変わります
            Delay_Ms(10);   // 更新間隔
        } 
        else {
            // --- ボタンが離された瞬間だけ：3つとも消灯 ---
            WS2812BSimpleSend(GPIOA, NEOPIXEL_PIN, (uint8_t[]){0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 9);
            Delay_Ms(10);   // 更新間隔
        }

        before_btn_state = current_btn_state;
    }
}

void usb_handle_user_data( struct usb_endpoint * e, int current_endpoint, uint8_t * data, int len, struct rv003usb_internal * ist )
{
	if (len > 0) {
		LogUEvent(1139, data[0], 0, current_endpoint);
	}
}

uint8_t tik = 0;
void usb_handle_user_in_request( struct usb_endpoint * e, uint8_t * scratchpad, int endp, uint32_t sendtok, struct rv003usb_internal * ist )
{
	if( endp == 1 )
	{
		// Mouse (4 bytes)
		// static int i;
		// static uint8_t tsajoystick[4] = { 0x00, 0x00, 0x00, 0x00 };
		// i++;
		// int mode = i >> 5;

		// Move the mouse right, down, left and up in a square.
		// switch( mode & 3 )
		// {
		// case 0: tsajoystick[1] =  1; tsajoystick[2] = 0; break;
		// case 1: tsajoystick[1] =  0; tsajoystick[2] = 1; break;
		// case 2: tsajoystick[1] = -1; tsajoystick[2] = 0; break;
		// case 3: tsajoystick[1] =  0; tsajoystick[2] =-1; break;
		// }
		// usb_send_data( tsajoystick, 4, 0, sendtok );
    usb_send_empty(sendtok);
	}
	else if( endp == 2 )
	{
		// Keyboard (8 bytes)
		static uint8_t tsajoystick[8] = { 0x00 };
		usb_send_data( tsajoystick, 8, 0, sendtok );
    uint8_t btn = GPIO_digitalRead(BUTTON_PIN);

		// Press a Key every second or so.
		if(tik == 0 && btn == 1 && before_btn_state == 0) {
			tsajoystick[4] = 0x18; // 0x05 = "b"; 0x53 = NUMLOCK; 0x39 = CAPSLOCK;
      tik = 1;
		} else if (tik > 0) {
      if (tik == 1) {
        tsajoystick[4] = 0x08; // 0x05 = "b"; 0x53 = NUMLOCK; 0x39 = CAPSLOCK;
        tik += 1;
      } else if (tik == 2) {
        tsajoystick[4] = 0x06; // 0x05 = "b"; 0x53 = NUMLOCK; 0x39 = CAPSLOCK;
        tik = 0;
      }
    } else {
			tsajoystick[4] = 0;
		}
    before_btn_state = GPIO_digitalRead(BUTTON_PIN);
	}
	else
	{
		// If it's a control transfer, empty it.
		usb_send_empty( sendtok );
	}
}


