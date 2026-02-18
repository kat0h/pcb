#include "ch32fun.h"
#include <stdio.h>
#define INTERNAL_VREF_MV 1200
void adc_init_vref(void) {
  // ADCクロック分周: 48MHz/8 = 6MHz (14MHz以下にする)
  RCC->CFGR0 &= ~(0x1f << 11);
  RCC->CFGR0 |= (0x03 << 11);
  RCC->APB2PCENR |= RCC_APB2Periph_ADC1;
  // ADCリセット
  RCC->APB2PRSTR |= RCC_APB2Periph_ADC1;
  RCC->APB2PRSTR &= ~RCC_APB2Periph_ADC1;
  ADC1->RSQR3 = 8; // チャンネル8(Vref)を選択
  ADC1->SAMPTR2 =
      (7 << (3 *
             8)); // サンプリング時間を最大(241.5 cycles)に設定して安定させる
  ADC1->CTLR2 |= ADC_ADON | ADC_EXTSEL; // ADC ON
  // 校正
  ADC1->CTLR2 |= ADC_RSTCAL;
  while (ADC1->CTLR2 & ADC_RSTCAL)
    ;
  ADC1->CTLR2 |= ADC_CAL;
  while (ADC1->CTLR2 & ADC_CAL)
    ;
}
uint32_t get_vcc_mv(void) {
  uint16_t samples[10];
  uint32_t sum = 0;
  uint16_t min_val = 1024;
  uint16_t max_val = 0;
  for (int i = 0; i < 10; i++) {
    ADC1->CTLR2 |= ADC_SWSTART;
    while (!(ADC1->STATR & ADC_EOC))
      ;
    samples[i] = ADC1->RDATAR;
    sum += samples[i];
    if (samples[i] < min_val)
      min_val = samples[i];
    if (samples[i] > max_val)
      max_val = samples[i];
    Delay_Ms(1);
  }
  uint32_t avg_vref_adc = (sum - max_val - min_val) / 8;
  if (avg_vref_adc == 0)
    return 0;
  uint32_t vcc_mv = (INTERNAL_VREF_MV * 1024) / avg_vref_adc;
  return vcc_mv;
}
