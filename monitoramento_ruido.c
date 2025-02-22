#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "pico/binary_info.h"
#include "inc/ssd1306.h"
#include "hardware/i2c.h"
#include "inc/mic.h"
#include "inc/pwm_buzzer.h"

// Pinos para comunicação I2C para o display OLED
const uint I2C_SDA = 14;
const uint I2C_SCL = 15;

// Define os pinos dos buzzer
#define BUZZER_1_PIN 10
#define BUZZER_2_PIN 21

// Frequência
#define FREQ_AGUDO 400

void init_ssd1306()
{

    // Inicialização do i2c
    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Processo de inicialização completo do OLED SSD1306
    ssd1306_init();
}

int main()
{
    stdio_init_all();

    // ###################################################################################

    // Delay para o usuário abrir o monitor serial...
    sleep_ms(5000);

    // Preparação do ADC.
    printf("Preparando ADC...\n");

    adc_gpio_init(MIC_PIN);
    adc_init();
    adc_select_input(MIC_CHANNEL);

    adc_fifo_setup(
        true,  // Habilitar FIFO
        true,  // Habilitar request de dados do DMA
        1,     // Threshold para ativar request DMA é 1 leitura do ADC
        false, // Não usar bit de erro
        false  // Não fazer downscale das amostras para 8-bits, manter 12-bits.
    );
    adc_set_clkdiv(ADC_CLOCK_DIV);

    printf("ADC Configurado!\n\n");
    printf("Preparando DMA...");

    // Tomando posse de canal do DMA.
    dma_channel = dma_claim_unused_channel(true);
    // Configurações do DMA.
    dma_cfg = dma_channel_get_default_config(dma_channel);
    // Tamanho da transferência é 16-bits (usamos uint16_t para armazenar valores do ADC)
    channel_config_set_transfer_data_size(&dma_cfg, DMA_SIZE_16);
    // Desabilita incremento do ponteiro de leitura (lemos de um único registrador)
    channel_config_set_read_increment(&dma_cfg, false);
    // Habilita incremento do ponteiro de escrita (escrevemos em um array/buffer)
    channel_config_set_write_increment(&dma_cfg, true);
    channel_config_set_dreq(&dma_cfg, DREQ_ADC); // Usamos a requisição de dados do ADC

    // Amostragem de teste.
    printf("Amostragem de teste...\n");
    sample_mic();

    printf("Configuracoes completas!\n");

    // ###################################################################################

    // Parte do código para exibir a mensagem no display (opcional: mudar ssd1306_height para 32 em ssd1306_i2c.h)
    // /**

    init_ssd1306();

    pwm_init_buzzer(BUZZER_2_PIN);

    // Preparar área de renderização para o display (ssd1306_width pixels por ssd1306_n_pages páginas)
    struct render_area frame_area = {
        start_column : 0,
        end_column : ssd1306_width - 1,
        start_page : 0,
        end_page : ssd1306_n_pages - 1
    };

    calculate_render_area_buffer_length(&frame_area);

    // zera o display inteiro
    uint8_t ssd[ssd1306_buffer_length];
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);

restart:

    char *text[] = {
        "      ",
        "      ",
        "  Monitor de Ruido!   ",
        "      ",
        "  Embarcatech   "};

    int y = 0;
    for (uint i = 0; i < count_of(text); i++)
    {
        ssd1306_draw_string(ssd, 5, y, text[i]);
        y += 8;
    }
    render_on_display(ssd, &frame_area);
    sleep_ms(3000);

    // zera o display inteiro
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);

    while (true)
    {
        // Realiza uma amostragem do microfone.
        sample_mic();
        // Pega a potência média da amostragem do microfone.
        float avg_rms = mic_power();
        // Ajusta para intervalo de 0 a 3.3V. (apenas magnitude, sem sinal)
        avg_rms = 2.f * abs(ADC_ADJUST(avg_rms));
        float db = rms_to_db(avg_rms);

        char nivel_avg[50];
        char *msg_db;

        if (db > 85)
        {
            msg_db = "Use Protetor Auricular!!!";
            play_tone(BUZZER_2_PIN, FREQ_AGUDO, 1000);
        }
        else
        {
            msg_db = "Anbiente Seguro!";
        }
        // Formata a string com o valor de db
        sprintf(nivel_avg, "Nivel dB:%.2fdB", db);

        char *text[] = {
            "      ",
            "      ",
            msg_db,
            "      ",
            nivel_avg,
        };

        int y = 0;
        for (uint i = 0; i < count_of(text); i++)
        {
            ssd1306_draw_string(ssd, 5, y, text[i]);
            y += 8;
        }
        render_on_display(ssd, &frame_area);
        printf("Nível de dB: %.2f\n", db); // Exibe o valor em dB
        sleep_ms(500);
    }
    return 0;
}
