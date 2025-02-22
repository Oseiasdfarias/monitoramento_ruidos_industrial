#ifndef BUZZER_H
#define BUZZER_H

#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "pico/stdlib.h"

// Inicializa o PWM no pino do buzzer
void pwm_init_buzzer(uint pin);

// Toca uma nota com a frequência e duração especificadas
void play_tone(uint pin, uint frequency, uint duration_ms);

#endif // BUZZER_H