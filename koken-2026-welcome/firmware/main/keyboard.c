uint8_t blue[6]  = {0, 0, 25, 0, 0, 25};
uint8_t red[6]   = {0, 25, 0, 0, 25, 0};
uint8_t black[6] = {0, 0, 0, 0, 0, 0};

uint8_t state = 0;
uint8_t count = 0;
uint8_t code  = 0;
uint8_t keycode_to_send = 0;

// https://github.com/ibuki2003/mini_cw_keyboard/blob/master/program/src/morse.c
const uint8_t morse_table[128] = {
  [  1] = 0x08, // E
  [  2] = 0x17, // T
  [  3] = 0x0c, // I
  [  4] = 0x04, // A
  [  5] = 0x11, // N
  [  6] = 0x10, // M
  [  7] = 0x16, // S
  [  8] = 0x18, // U
  [  9] = 0x15, // R
  [ 10] = 0x1a, // W
  [ 11] = 0x07, // D
  [ 12] = 0x0e, // K
  [ 13] = 0x0a, // G
  [ 14] = 0x12, // O
  [ 15] = 0x0b, // H
  [ 16] = 0x19, // V
  [ 17] = 0x09, // F
  [ 19] = 0x0f, // L
  [ 21] = 0x13, // P
  [ 22] = 0x0d, // J
  [ 23] = 0x05, // B
  [ 24] = 0x1b, // X
  [ 25] = 0x06, // C
  [ 26] = 0x1c, // Y
  [ 27] = 0x1d, // Z
  [ 28] = 0x14, // Q
  [ 31] = 0x22, // 5
  [ 32] = 0x21, // 4
  [ 34] = 0x20, // 3
  [ 38] = 0x1f, // 2
  [ 46] = 0x1e, // 1
  [ 47] = 0x23, // 6
  [ 55] = 0x24, // 7
  [ 59] = 0x25, // 8
  [ 61] = 0x26, // 9
  [ 62] = 0x27, // 0
  [ 18] = 0x2c, // SPC: ..--
  [ 20] = 0x28, // Return: .-.-
  [ 33] = 0x2a, // BS: ...-.
};

void keyboard_main() {
  Delay_Ms(1);
  usb_setup();
  uint8_t v; // button state
  while (1) {
    if (keycode_to_send != 0) {
      Delay_Ms(10);
      continue;
    }
    v = !GPIO_digitalRead(BT_PIN);
    switch (state) {
      case 0:
        if (v) {
          state = 1;
          count = 0;
        } else {
          count += 10;
        }
        if (count > 250 && code != 0) {
          // next char
          WS2812BSimpleSend(GPIOA, NP_PIN, black, 6);
          // submit char
          keycode_to_send = morse_table[code];
          code = 0;
        }
        break;
      case 1:
        if (v) {
          count += 10;
        } else {
          state = 0;
          // dot
          WS2812BSimpleSend(GPIOA, NP_PIN, blue, 6);
          code = (code << 1) + 1;
        }
        if (count > 150)
          state = 2;
        break;
      case 2:
        if (!v) {
          state = 0;
          // dash
          WS2812BSimpleSend(GPIOA, NP_PIN, red, 6);
          code = (code << 1) + 2;
        }
        break;
    }
    Delay_Ms(10);
  }
}

void usb_handle_user_data(struct usb_endpoint *e, int current_endpoint,
                          uint8_t *data, int len,
                          struct rv003usb_internal *ist) {}

uint8_t cnt = 0;
void usb_handle_user_in_request(struct usb_endpoint *e, uint8_t *scratchpad,
                                int endp, uint32_t sendtok,
                                struct rv003usb_internal *ist) {
  if (endp == 2) {
    static uint8_t tsajoystick[8] = { 0x00 };
    if (keycode_to_send != 0) {
      tsajoystick[4] = keycode_to_send;
      keycode_to_send = 0;
      cnt = 10;
    } else if (cnt == 0) {
      tsajoystick[4] = 0;
    } else {
      cnt -= 1;
    }
    usb_send_data(tsajoystick, 8, 0, sendtok);
  } else usb_send_empty(sendtok);
}
