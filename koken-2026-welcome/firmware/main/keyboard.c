uint8_t state = 0;
uint8_t blue[6]  = {0, 0, 255, 0, 0, 255};
uint8_t red[6]   = {0, 255, 0, 0, 255, 0};
uint8_t black[6] = {0, 0, 0, 0, 0, 0};

void keyboard_main() {
  uint8_t v; // button state
  while (1) {
    v = !GPIO_digitalRead(BT_PIN);
    if (v) WS2812BSimpleSend(GPIOA, NP_PIN, blue, 6);
    else WS2812BSimpleSend(GPIOA, NP_PIN, black, 6);
    Delay_Ms(10);
  }
}

void usb_handle_user_data(struct usb_endpoint *e, int current_endpoint,
                          uint8_t *data, int len,
                          struct rv003usb_internal *ist) {}
void usb_handle_user_in_request(struct usb_endpoint *e, uint8_t *scratchpad,
                                int endp, uint32_t sendtok,
                                struct rv003usb_internal *ist) {
  usb_send_empty(sendtok);
}

