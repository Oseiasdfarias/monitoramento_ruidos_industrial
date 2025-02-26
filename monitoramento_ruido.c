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

// Pinos para comunicação I2C com o display OLED
const uint I2C_SDA = 14; // Pino de dados (SDA) para I2C
const uint I2C_SCL = 15; // Pino de clock (SCL) para I2C

// Pinos GPIO para os buzzers
#define BUZZER_1_PIN 10 // Pino do primeiro buzzer
#define BUZZER_2_PIN 21 // Pino do segundo buzzer

// Frequência do tom agudo para o buzzer
#define FREQ_AGUDO 400 // Frequência em Hz

// Função para inicializar o display OLED SSD1306
void init_ssd1306()
{
    // Inicializa a comunicação I2C com o display
    i2c_init(i2c1, ssd1306_i2c_clock * 1000);  // Configura o clock do I2C
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // Define o pino SDA como função I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // Define o pino SCL como função I2C
    gpio_pull_up(I2C_SDA);                     // Habilita pull-up no pino SDA
    gpio_pull_up(I2C_SCL);                     // Habilita pull-up no pino SCL

    // Inicializa o display OLED
    ssd1306_init();
}

// Função principal
int main()
{
    // Inicializa todas as funcionalidades padrão do Pico
    stdio_init_all();

    // Aguarda 5 segundos para permitir a abertura do monitor serial
    sleep_ms(5000);

    // Configuração do ADC para leitura do microfone
    printf("Preparando ADC...\n");
    adc_gpio_init(MIC_PIN);        // Inicializa o pino do microfone como entrada analógica
    adc_init();                    // Inicializa o ADC
    adc_select_input(MIC_CHANNEL); // Seleciona o canal do ADC para o microfone

    // Configuração do FIFO do ADC
    adc_fifo_setup(
        true,  // Habilita o FIFO
        true,  // Habilita o request de dados do DMA
        1,     // Threshold para ativar o DMA (1 leitura)
        false, // Não usa bit de erro
        false  // Mantém as amostras em 12 bits (não faz downscale para 8 bits)
    );
    adc_set_clkdiv(ADC_CLOCK_DIV); // Define o divisor de clock do ADC

    printf("ADC Configurado!\n\n");
    printf("Preparando DMA...");

    // Configuração do DMA para transferência eficiente dos dados do ADC
    dma_channel = dma_claim_unused_channel(true);                 // Obtém um canal de DMA disponível
    dma_cfg = dma_channel_get_default_config(dma_channel);        // Obtém a configuração padrão do DMA
    channel_config_set_transfer_data_size(&dma_cfg, DMA_SIZE_16); // Define o tamanho da transferência como 16 bits
    channel_config_set_read_increment(&dma_cfg, false);           // Desabilita incremento do ponteiro de leitura
    channel_config_set_write_increment(&dma_cfg, true);           // Habilita incremento do ponteiro de escrita
    channel_config_set_dreq(&dma_cfg, DREQ_ADC);                  // Usa a requisição de dados do ADC

    // Realiza uma amostragem de teste do microfone
    printf("Amostragem de teste...\n");
    sample_mic();

    printf("Configuracoes completas!\n");

    // Inicializa o display OLED
    init_ssd1306();

    // Inicializa o buzzer
    pwm_init_buzzer(BUZZER_2_PIN);

    // Define a área de renderização para o display OLED
    struct render_area frame_area = {
        start_column : 0,               // Coluna inicial
        end_column : ssd1306_width - 1, // Coluna final
        start_page : 0,                 // Página inicial
        end_page : ssd1306_n_pages - 1  // Página final
    };

    // Calcula o tamanho do buffer de renderização
    calculate_render_area_buffer_length(&frame_area);

    // Cria um buffer para o display e o preenche com zeros (limpa o display)
    uint8_t ssd[ssd1306_buffer_length];
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);

// Label para reiniciar a exibição da mensagem inicial
restart:

    // Mensagem inicial para exibir no display OLED
    char *text[] = {
        "      ",
        "      ",
        "  Monitor de Ruido!   ",
        "      ",
        "  Embarcatech   "};

    // Exibe a mensagem no display OLED
    int y = 0;
    for (uint i = 0; i < count_of(text); i++)
    {
        ssd1306_draw_string(ssd, 5, y, text[i]); // Desenha cada linha da mensagem
        y += 8;                                  // Move para a próxima linha
    }
    render_on_display(ssd, &frame_area); // Renderiza a mensagem no display
    sleep_ms(3000);                      // Aguarda 3 segundos

    // Limpa o display
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);

    // Loop principal do programa
    while (true)
    {
        // Realiza uma amostragem do microfone
        sample_mic();

        // Calcula a potência média do sinal captado pelo microfone
        float avg_rms = mic_power();

        // Ajusta o valor RMS para o intervalo de 0 a 3.3V (magnitude absoluta)
        avg_rms = 2.f * abs(ADC_ADJUST(avg_rms));

        // Converte o valor RMS para decibéis (dB)
        float db = rms_to_db(avg_rms);

        // Buffer para armazenar o nível de dB formatado
        char nivel_avg[50];
        char *msg_db;

        // Verifica se o nível de dB ultrapassa 85 dB
        if (db > 85)
        {
            msg_db = "Use Protetor!!!"; // Mensagem de alerta
            // Formata a string com o valor de dB
            sprintf(nivel_avg, "Nivel dB:%d dB", (int)db);

            // Texto para exibir no display OLED
            char *text[] = {
                "      ",
                "      ",
                msg_db, // Mensagem de alerta ou segurança
                "      ",
                nivel_avg // Nível de dB formatado
            };

            // Exibe o texto no display OLED
            int y = 0;
            for (uint i = 0; i < count_of(text); i++)
            {
                ssd1306_draw_string(ssd, 5, y, text[i]); // Desenha cada linha
                y += 8;                                  // Move para a próxima linha
            }
            render_on_display(ssd, &frame_area);       // Renderiza o texto no display
            play_tone(BUZZER_2_PIN, FREQ_AGUDO, 1000); // Toca um tom agudo no buzzer
        }
        else
        {
            msg_db = "Ambiente Seguro!"; // Mensagem de segurança

            // Formata a string com o valor de dB
            sprintf(nivel_avg, "Nivel dB:%d dB", (int)db);

            // Texto para exibir no display OLED'
            char *text[] = {
                "      ",
                "      ",
                msg_db, // Mensagem de alerta ou segurança
                "      ",
                nivel_avg // Nível de dB formatado
            };

            // Exibe o texto no display OLED
            int y = 0;
            for (uint i = 0; i < count_of(text); i++)
            {
                ssd1306_draw_string(ssd, 5, y, text[i]); // Desenha cada linha
                y += 8;                                  // Move para a próxima linha
            }
            render_on_display(ssd, &frame_area); // Renderiza o texto no display
        }

        // Exibe o valor de dB no monitor serial
        printf("Nível de dB: %.2f\n", db);

        // Aguarda 500 ms antes de repetir o loop
        sleep_ms(20);
    }

    return 0;
}