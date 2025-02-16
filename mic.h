#ifndef MIC_H
#define MIC_H

#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/dma.h"

// Definição do canal do microfone no ADC.
#define MIC_CHANNEL 2
#define MIC_PIN (26 + MIC_CHANNEL)

// Parâmetros do ADC.
#define ADC_CLOCK_DIV 96.f
#define SAMPLES 200
#define ADC_ADJUST(x) (x * 3.3f / (1 << 12u) - 1.65f)
#define ADC_MAX 3.3f
#define ADC_STEP (3.3f / 5.f)


#define abs(x) ((x < 0) ? (-x) : (x))

// Declaração das variáveis globais.
extern uint dma_channel;
extern dma_channel_config dma_cfg;
extern uint16_t adc_buffer[SAMPLES];

// Declaração das funções.
void sample_mic();
float mic_power();
uint8_t get_intensity(float v);

#endif // MIC_H
