#define BRIGHTNESS 10

// 虹色計算用の変数
uint8_t wheel_pos = 0;
// HSVからRGB（正確にはWS2812B用のGRB）に変換する簡易関数
// led_index: 現在設定するLEDの番号 (0, 1, 2)
// total_leds: 全体のLED数
// pos: 色相の基準値 (0-255)
void get_rainbow_for_led(uint8_t led_index, uint8_t total_leds,
                         uint8_t base_pos, uint8_t *grb) {
  // 各LEDで色相をずらす
  // base_posからのオフセットを計算
  uint8_t pos = (base_pos + (led_index * (255 / total_leds))) % 255;

  pos = 255 - pos; // 色相の方向を調整
  if (pos < 85) {
    grb[0] = 255 - pos * 3;
    grb[1] = 0;
    grb[2] = pos * 3; // G, R, B
  } else if (pos < 170) {
    pos -= 85;
    grb[0] = 0;
    grb[1] = pos * 3;
    grb[2] = 255 - pos * 3;
  } else {
    pos -= 170;
    grb[0] = pos * 3;
    grb[1] = 255 - pos * 3;
    grb[2] = 0;
  }
  grb[0] = (grb[0] * BRIGHTNESS) >> 8; // Green
  grb[1] = (grb[1] * BRIGHTNESS) >> 8; // Red
  grb[2] = (grb[2] * BRIGHTNESS) >> 8; // Blue
}
