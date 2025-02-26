#ifndef LED_MATRIZ_H
#define LED_MATRIZ_H

#include <stdbool.h>
#include <stdint.h>

#define LED_CONTAGEM 25

// Declaração dos buffers para armazenar os números de 0 a 9
extern bool simbolo_perigo[LED_CONTAGEM];

// Função para configurar os LEDs conforme um número específico
void set_one_led(uint8_t r, uint8_t g, uint8_t b, bool numero_desenhado[]);

// Função para exibir o número baseado no contador
void mostra_numero_baseado_no_contador();

#endif // MATRIZ_LED_H