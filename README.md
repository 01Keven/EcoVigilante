# Introdução
O sistema tem como principal objetivo acompanhar as condições dos abrigos artificiais (Bat Houses) e locais propensos à presença desses animais, como telhados e sótãos. Com isso, tem-se o objetivo de garantir um ambiente adequado para os morcegos e, ao mesmo tempo, reduzir riscos à saúde pública, prevenindo doenças como a histoplasmose, que pode ser transmitida pelas fezes desses animais e propagação do vírus da raiva, por animais portadores do mesmo.

# Dependências e Bibliotecas
O projeto requer as seguintes bibliotecas:
- `pico/stdlib.h`: Biblioteca padrão para desenvolvimento no Raspberry Pi Pico.
- `hardware/pwm.h`: Para controle de sinais PWM.
- `hardware/clocks.h`: Para manipulação dos clocks do sistema.
- `hardware/adc.h`: Para leitura de entradas analógicas (joystick, sensores).
- `hardware/i2c.h`: Para comunicação com dispositivos I2C (display SSD1306).
- `inc/ssd1306.h`: Biblioteca para controle do display OLED.
- `inc/font.h`: Conjunto de fontes para o display.
- `ws2812.pio.h`: Biblioteca para controle de LEDs endereçáveis.
- `inc/led_matriz.h`: Biblioteca para exibição de caracteres na matriz de LEDs.

# Definição de Pinos
Os pinos utilizados no projeto são:
- **Matriz de LEDs**: Pino 7
- **I2C**: SDA (pino 14) e SCL (pino 15)
- **Display OLED (SSD1306)**: Endereço I2C 0x3C
- **Botões**: Botão A (pino 5), Botão B (pino 6), Botão do Joystick (pino 22)
- **LEDs RGB**: Verde (pino 11), Azul (pino 12), Vermelho (pino 13)
- **Joystick**: X (pino 26), Y (pino 27), com zona morta de 40
- **Buzzer**: Pino 21, com frequência padrão de 1000Hz

# Variáveis Globais
Definição das variáveis utilizadas:
- `pwm_enabled`: Flag para controle do PWM
- `last_button_a_time`, `last_button_joy_time`: Controle de debounce dos botões
- `joystick_activated`: Estado do joystick
- `morcegos`: Quantidade de morcegos detectados
- `temperatura`: Valor inicial da temperatura
- `is_temperature_locked`: Flag para fixar a temperatura
- `qualidade_ar`: Qualidade do ar inicial
- `morcegos_detectados`: Contagem de morcegos
- `alerta_ativo`: Flag para estado do alerta
- `tempo_inicio`: Tempo de início do alerta

# Funções Principais

## `gerar_morcegos()`
Gera um número aleatório de morcegos entre 10 e 100.

## `pwm_setup(uint pin)`
Configura um pino para gerar sinal PWM.

## `pwm_buzzer_setup(uint pin, int frequency)`
Inicializa o buzzer e ajusta a frequência.

## `read_adc(uint channel)`
Lê um valor do ADC para entrada analógica.

## `map_adc_to_screen(int adc_value, int center_value, int screen_max)`
Mapeia os valores do ADC para a tela OLED.

## `gpio_irq_handler(uint gpio, uint32_t events)`
Interrupção de GPIO para capturar eventos dos botões.

## `update_temperature()`
Atualiza a temperatura conforme o movimento do joystick e controla os LEDs e buzzer com base na temperatura.

## `exibir_dados_oled()`
Exibe informações sobre temperatura, qualidade do ar e morcegos detectados na tela OLED.

## `controlar_alerta()`
Ativa um alerta visual e sonoro se um evento crítico for detectado.

---

## **Bibliotecas Incluídas**
```c
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "hardware/adc.h"
#include <stdlib.h>  
#include "hardware/i2c.h"
#include "inc/ssd1306.h"
#include "inc/font.h"
#include "ws2812.pio.h"
#include "inc/led_matriz.h"
#include <time.h>
#include <stdint.h>
#include <stdbool.h>
```
### **Descrição**
Este trecho inclui bibliotecas essenciais para o funcionamento do código:
- **stdio.h** → Permite entrada e saída padrão (`printf`, `scanf`).
- **pico/stdlib.h** → Biblioteca padrão para Raspberry Pi Pico.
- **hardware/pwm.h** → Controle de PWM (Pulse Width Modulation).
- **hardware/clocks.h** → Manipulação dos clocks do microcontrolador.
- **hardware/adc.h** → Leitura de entradas analógicas (ADC).
- **stdlib.h** → Funções como `rand()` (geração de números aleatórios).
- **hardware/i2c.h** → Comunicação via protocolo I2C.
- **ssd1306.h** e **font.h** → Manipulação do display OLED SSD1306.
- **ws2812.pio.h** → Controle de LEDs endereçáveis WS2812.
- **led_matriz.h** → Dados para exibir caracteres na matriz LED.
- **time.h, stdint.h, stdbool.h** → Manipulação de tempo, tipos de dados e booleanos.

---

## **Definições de Pinos e Parâmetros**
```c
#define MATRIZ_LED_PIN 7  
#define I2C_PORT i2c1  
#define I2C_SDA 14     
#define I2C_SCL 15     
#define SSD1306_ADDR 0x3C  
#define BUTTON_A 5     
#define BUTTON_B 6     
#define BUTTON_JOY 22  
#define LED_GREEN 11   
#define LED_BLUE 12    
#define LED_RED 13     
#define JOYSTICK_X_PIN 26   
#define JOYSTICK_Y_PIN 27   
#define JOYSTICK_DEADZONE 40  
#define BUZZER_PIN 21  
#define BUZZER_FREQUENCY 1000  
#define JOYSTICK_CENTER_X 1939  
#define JOYSTICK_CENTER_Y 2180  
#define SCREEN_WIDTH 128  
#define SCREEN_HEIGHT 64  
```
### **Descrição**
- Define os pinos GPIO conectados a cada componente do sistema, como botões, LEDs, joystick, buzzer, display OLED e matriz de LEDs.
- `JOYSTICK_DEADZONE` define uma zona morta onde pequenos movimentos não são considerados.
- `BUZZER_FREQUENCY` determina a frequência do som do buzzer.

---

## **Declaração de Variáveis Globais**
```c
static volatile bool pwm_enabled = true;  
static volatile uint32_t last_button_a_time = 0;  
static volatile uint32_t last_button_joy_time = 0;  
static volatile bool joystick_activated = false;
static volatile int morcegos = 50; 
static volatile bool is_temperature_locked = false;
static volatile bool is_qualidade_ar_locked = false;
int temperatura = 0;
int qualidade_ar = 50;
int qualidade_ar_max = 100;
int qualidade_ar_min = 0;
volatile int morcegos_detectados = 0;
volatile bool alerta_ativo = false;
volatile time_t tempo_inicio = 0;
```
### **Descrição**
- **Variáveis `volatile`** → Indicam que podem ser modificadas por interrupções.
- `pwm_enabled` → Controla se o PWM está ativo.
- `last_button_a_time` / `last_button_joy_time` → Armazena o tempo da última ativação de botões, evitando acionamentos repetidos (debounce).
- `joystick_activated` → Indica se o joystick foi movido além da zona morta.
- `morcegos` → Número de morcegos simulados no sistema.
- `is_temperature_locked` → Bloqueia ou libera a mudança da temperatura.
- `is_qualidade_ar_locked` → Bloqueia ou libera a alteração da qualidade do ar.
- `temperatura` → Controla a temperatura simulada do sistema.
- `qualidade_ar` → Representa a qualidade do ar, entre 0 e 100.
- `morcegos_detectados` → Contagem de morcegos detectados.
- `alerta_ativo` → Indica se um alerta está em andamento.
- `tempo_inicio` → Marca o tempo do início de um alerta.

---

## **Geração Aleatória de Morcegos**
```c
int gerar_morcegos() {
    return (rand() % 171) + 10;  
}
```
### **Descrição**
- Gera um número aleatório de morcegos entre 10 e 180.
- Usa `rand()` para criar variações aleatórias.

---

## **Configuração do PWM**
```c
void pwm_setup(uint pin) {
    gpio_set_function(pin, GPIO_FUNC_PWM);  
    uint slice = pwm_gpio_to_slice_num(pin); 
    pwm_set_clkdiv(slice, 4.0);  
    pwm_set_wrap(slice, 65535);  
    pwm_set_gpio_level(pin, 0);  
    pwm_set_enabled(slice, true); 
}
```
### **Descrição**
- Ativa PWM em um pino específico.
- Define um divisor de clock e a resolução máxima de 16 bits (`65535`).

---

## **Configuração do PWM no Buzzer**
```c
void pwm_buzzer_setup(uint pin, int frequency) {
    pwm_setup(pin);  
    uint slice = pwm_gpio_to_slice_num(pin);  
    uint32_t pwm_clock = clock_get_hz(clk_sys);  
    uint32_t clkdiv = (float)pwm_clock / (frequency * 65536);  
    pwm_set_clkdiv(slice, clkdiv);  
    pwm_set_wrap(slice, 65535);  
}
```
### **Descrição**
- Configura o PWM para gerar sons no buzzer.
- Ajusta a frequência do som conforme necessário.

---

## **Leitura de Sensores Analógicos (ADC)**
```c
uint16_t read_adc(uint channel) {
    adc_select_input(channel);  
    return adc_read();  
}
```
### **Descrição**
- Lê valores do ADC de um determinado canal.
- Usado para capturar dados do joystick.

---

## **Mapeamento do ADC para Tela**
```c
int map_adc_to_screen(int adc_value, int center_value, int screen_max) {
    int range_min = center_value;     
    int range_max = 4095 - center_value;  
    int offset = adc_value - center_value;
    int mapped_value;

    if (offset < 0) {
        mapped_value = ((offset * (screen_max / 2)) / range_min) + (screen_max / 2);
    } else {
        mapped_value = ((offset * (screen_max / 2)) / range_max) + (screen_max / 2);
    }

    if (mapped_value < 0) mapped_value = 0;
    if (mapped_value > screen_max) mapped_value = screen_max;

    return mapped_value;
}
```
### **Descrição**
- Mapeia valores do ADC para coordenadas utilizáveis na tela OLED.
- Converte variações analógicas em deslocamentos úteis para a interface.

---

## **Interrupção dos Botões**
```c
static void gpio_irq_handler(uint gpio, uint32_t events) {
    if (gpio == BUTTON_A) {
        uint32_t current_time = time_us_32();
        if (current_time - last_button_a_time > 200000) { 
            last_button_a_time = current_time;
            is_temperature_locked = !is_temperature_locked;
        }
    }
    if (gpio == BUTTON_B) {
        morcegos = gerar_morcegos();
        printf("Botão pressionado! Morcegos atualizados para %d\n", morcegos);
    }
}
```
### **Descrição**
- Garante debounce ao verificar se 200ms se passaram desde o último clique.
- Alterna o bloqueio da temperatura e atualiza o número de morcegos.

---
