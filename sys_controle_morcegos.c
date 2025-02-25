#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "inc/ssd1306.h"
#include "inc/font.h"

// Definições dos pinos e parâmetros
#define I2C_PORT i2c1  // Porta I2C utilizada
#define I2C_SDA 14     // Pino SDA do I2C
#define I2C_SCL 15     // Pino SCL do I2C
#define SSD1306_ADDR 0x3C  // Endereço do display SSD1306
#define BUTTON_A 5     // Pino do botão A
#define BUTTON_B 6     // Pino do botão B
#define BUTTON_JOY 22  // Pino do botão do joystick
#define LED_GREEN 11   // Pino do LED verde
#define LED_BLUE 12    // Pino do LED azul
#define LED_RED 13     // Pino do LED vermelho
#define JOYSTICK_X_PIN 26   // Pino do eixo X do joystick
#define JOYSTICK_Y_PIN 27   // Pino do eixo Y do joystick
#define JOYSTICK_DEADZONE 40  // Zona morta do joystick

// Ajuste do centro do joystick
#define JOYSTICK_CENTER_X 1939 // Valor de centro do eixo X
#define JOYSTICK_CENTER_Y 2180 // Valor de centro do eixo Y

// Parâmetros da tela e exibição
#define SCREEN_WIDTH 128 // Largura da tela
#define SCREEN_HEIGHT 64 // Altura da tela

// Variáveis globais
static volatile bool pwm_enabled = true;  // Flag para controlar o PWM
static volatile uint32_t last_button_a_time = 0;  // Controle de debounce do botão A
static volatile uint32_t last_button_joy_time = 0;  // Controle de debounce do botão do joystick
static volatile bool is_border_thick = false;  // Alterna a espessura da borda
static volatile bool is_square_red = true;    // Controle da cor do quadrado

// Variável para armazenar a temperatura
int temperatura = 20;  // Temperatura inicial, por exemplo, 20°C

// Prototipos das funções de interrupção
static void gpio_irq_handler(uint gpio, uint32_t events);

// Inicialização do PWM
void pwm_setup(uint pin) {
    gpio_set_function(pin, GPIO_FUNC_PWM);  // Configura o pino como PWM
    uint slice = pwm_gpio_to_slice_num(pin); // Obtém o slice do pino
    pwm_set_clkdiv(slice, 4.0);  // Configura o divisor de clock do PWM
    pwm_set_wrap(slice, 65535);  // Define o valor máximo do contador PWM
    pwm_set_gpio_level(pin, 0);  // Inicializa o nível do PWM em 0 (desligado)
    pwm_set_enabled(slice, true);// Habilita o PWM no slice
}

// Lê um valor do ADC
uint16_t read_adc(uint channel) {
    adc_select_input(channel);  // Seleciona o canal do ADC
    return adc_read();  // Lê o valor do ADC
}

// Mapeia os valores do ADC para a tela SSD1306
int map_adc_to_screen(int adc_value, int center_value, int screen_max) {
    int range_min = center_value;     // Valor mínimo da faixa de mapeamento
    int range_max = 4095 - center_value;  // Valor máximo da faixa de mapeamento
    
    int offset = adc_value - center_value; // Calcula o deslocamento do valor do ADC

    int mapped_value;
    if (offset < 0) {
        mapped_value = ((offset * (screen_max / 2)) / range_min) + (screen_max / 2); // Mapeia valores negativos
    } else {
        mapped_value = ((offset * (screen_max / 2)) / range_max) + (screen_max / 2); // Mapeia valores positivos
    }

    if (mapped_value < 0) mapped_value = 0; // Limita o valor mínimo
    if (mapped_value > screen_max) mapped_value = screen_max; // Limita o valor máximo

    return mapped_value;
}

// Atualiza a temperatura com base no movimento do joystick
void update_temperature() {
    uint16_t adc_y = read_adc(0);  // Lê o valor do eixo Y do joystick

    // Calcula o deslocamento do eixo Y
    int16_t offset_y = adc_y - JOYSTICK_CENTER_Y;

    // Verifica se o deslocamento ultrapassa a zona morta
    if (offset_y > JOYSTICK_DEADZONE) {
        temperatura += 1;  // Aumenta a temperatura
    } else if (offset_y < -JOYSTICK_DEADZONE) {
        temperatura -= 1;  // Diminui a temperatura
    }

    // Limita a temperatura dentro dos valores extremos
    if (temperatura < 0) temperatura = 0;
    if (temperatura > 50) temperatura = 50;
}


// Desenha a borda no display
void draw_border(ssd1306_t *ssd) {
    int thickness = is_border_thick ? 3 : 1; // Define a espessura da borda

    // Desenha as linhas horizontais e verticais para a borda
    for (int i = 0; i < thickness; i++) {
        ssd1306_hline(ssd, 0, SCREEN_WIDTH - 2, i, true);
        ssd1306_hline(ssd, 0, SCREEN_WIDTH - 2, SCREEN_HEIGHT - 2 - i, true);
        ssd1306_vline(ssd, i, 0, SCREEN_HEIGHT - 2, true);
        ssd1306_vline(ssd, SCREEN_WIDTH - 2 - i, 0, SCREEN_HEIGHT - 2, true);
    }
}

// Atualiza o display SSD1306
// Atualiza o display SSD1306
void update_display(ssd1306_t *ssd) {
    ssd1306_fill(ssd, false);  // Limpa a tela
    draw_border(ssd);  // Desenha a borda

    // Desenha a temperatura na tela
    char temp_str[16];
    snprintf(temp_str, sizeof(temp_str), "Temp: %d C", temperatura);
    ssd1306_draw_string(ssd, temp_str, 0, 0);  // Passa o ponteiro correto e remove o 'true'

    ssd1306_send_data(ssd);  // Envia os dados para o display
}


// Função de interrupção para o GPIO
static void gpio_irq_handler(uint gpio, uint32_t events) {
    if (gpio == BUTTON_A) {
        // Lógica do botão A
        uint32_t current_time = time_us_32();
        if (current_time - last_button_a_time > 200000) { // Evita debounce
            last_button_a_time = current_time;
            is_border_thick = !is_border_thick;  // Alterna a espessura da borda
        }
    }
    if (gpio == BUTTON_JOY) {
        // Lógica do botão do joystick
        uint32_t current_time = time_us_32();
        if (current_time - last_button_joy_time > 200000) { // Evita debounce
            last_button_joy_time = current_time;
            is_square_red = !is_square_red;  // Alterna a cor do quadrado
        }
    }
}

int main() {   
    stdio_init_all();  // Inicializa a comunicação padrão

    pwm_setup(LED_RED);  // Configura o PWM para o LED vermelho
    pwm_setup(LED_BLUE); // Configura o PWM para o LED azul

    gpio_init(LED_GREEN); // Inicializa o LED verde
    gpio_set_dir(LED_GREEN, GPIO_OUT);
    gpio_put(LED_GREEN, 0);  // Inicializa o LED verde apagado

    gpio_init(BUTTON_A);  // Inicializa o botão A
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);  // Habilita o pull-up para o botão A
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);  // Configura a interrupção para o botão A

    gpio_init(BUTTON_JOY); // Inicializa o botão do joystick
    gpio_set_dir(BUTTON_JOY, GPIO_IN);
    gpio_pull_up(BUTTON_JOY);  // Habilita o pull-up para o botão do joystick
    gpio_set_irq_enabled(BUTTON_JOY, GPIO_IRQ_EDGE_FALL, true);  // Configura a interrupção para o botão do joystick

    adc_init();  // Inicializa o ADC
    adc_gpio_init(JOYSTICK_X_PIN);  // Inicializa o pino do eixo X do joystick
    adc_gpio_init(JOYSTICK_Y_PIN);  // Inicializa o pino do eixo Y do joystick

    i2c_init(I2C_PORT, 400 * 1000);  // Inicializa a comunicação I2C
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);  // Configura os pinos SDA e SCL para I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    ssd1306_t ssd;  // Declaração da estrutura do display SSD1306
    ssd1306_init(&ssd, 128, 64, false, SSD1306_ADDR, I2C_PORT);  // Inicializa o display SSD1306
    ssd1306_config(&ssd);  // Configura o display
    ssd1306_send_data(&ssd);  // Envia os dados de configuração para o display
    ssd1306_fill(&ssd, false);  // Limpa a tela

    while (1) {
        update_temperature();  // Atualiza a temperatura com base no movimento do joystick
        update_display(&ssd);  // Atualiza o display com a nova temperatura

        sleep_ms(50);  // Aguarda um curto período de tempo
    }
}
