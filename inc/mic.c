#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/dma.h"

// Pino e canal do microfone no ADC.
#define MIC_CHANNEL 2
#define MIC_PIN (26 + MIC_CHANNEL)

// Parâmetros e macros do ADC.
#define ADC_CLOCK_DIV 96.f
#define SAMPLES 50                                    // Número de amostras que serão feitas do ADC.
#define ADC_ADJUST(x) (x * 3.3f / (1 << 12u) - 1.65f) // Ajuste do valor do ADC para Volts.
#define ADC_MAX 3.3f
#define ADC_STEP (3.3f / 5.f) // Intervalos de volume do microfone.

#define abs(x) ((x < 0) ? (-x) : (x))

// Canal e configurações do DMA
uint dma_channel;
dma_channel_config dma_cfg;

// Buffer de amostras do ADC.
uint16_t adc_buffer[SAMPLES];

void sample_mic();
float mic_power();
uint8_t get_intensity(float v);

/**
 * Realiza as leituras do ADC e armazena os valores no buffer.
 */
void sample_mic()
{
    adc_fifo_drain(); // Limpa o FIFO do ADC.
    adc_run(false);   // Desliga o ADC (se estiver ligado) para configurar o DMA.

    dma_channel_configure(dma_channel, &dma_cfg,
                          adc_buffer,      // Escreve no buffer.
                          &(adc_hw->fifo), // Lê do ADC.
                          SAMPLES,         // Faz "SAMPLES" amostras.
                          true             // Liga o DMA.
    );

    // Liga o ADC e espera acabar a leitura.
    adc_run(true);
    dma_channel_wait_for_finish_blocking(dma_channel);

    // Acabou a leitura, desliga o ADC de novo.
    adc_run(false);
}

/**
 * Calcula a potência média das leituras do ADC. (Valor RMS)
 */
float mic_power()
{
    float avg = 0.f;

    for (uint i = 0; i < SAMPLES; ++i)
        avg += adc_buffer[i] * adc_buffer[i];

    avg /= SAMPLES;
    return sqrt(avg);
}

/**
 * Converte o valor RMS em decibéis (dB).
 */
float rms_to_db(float rms_value)
{
    float v_ref = 0.00002f; // Tensão de referência de 1.0 Volt.
    return 20.0f * log10f(rms_value / v_ref);
}