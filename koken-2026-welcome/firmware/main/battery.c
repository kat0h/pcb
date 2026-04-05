// 1. AWU用の割り込みハンドラを定義する (これがないと復帰時にフリーズします)
// __attribute__((interrupt)) はRISC-Vの割り込み処理に必須です
void AWU_IRQHandler(void) __attribute__((interrupt));
void AWU_IRQHandler(void) { // EXTI Line 9 (AWU) の割り込みフラグをクリア EXTI->INTFR = (1 << 9);
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
  uint8_t all_led_data[6] = {0};
  while (1) {
    uint8_t brightness = !GPIO_digitalRead(BT_PIN) ? 50 : 5;
    wheel_pos += 4;
    for (int i = 0; i < 2; i++) {
      // 各LEDの色相をずらす
      uint8_t hue = wheel_pos + (i * (255 / 4));
      uint32_t color = EHSVtoHEX(hue, 255, brightness);
      all_led_data[i * 3 + 0] = (color >> 8) & 0xFF;
      all_led_data[i * 3 + 1] = (color >> 0) & 0xFF;
      all_led_data[i * 3 + 2] = (color >> 16) & 0xFF;
    }
    WS2812BSimpleSend(GPIOA, NP_PIN, all_led_data, 6);
    enter_stop_mode_30ms();
  }
}
