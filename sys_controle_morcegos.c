#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "hardware/adc.h"
#include <stdlib.h>  // Para usar rand()
#include "hardware/i2c.h"
#include "inc/ssd1306.h"
#include "inc/font.h"
#include "ws2812.pio.h"
#include "inc/led_matriz.h"// Onde estão os caracteres armazenados para mostrar no display
#include <time.h>
#include <stdint.h>
#include <stdbool.h>


// Definições dos pinos e parâmetros
#define MATRIZ_LED_PIN 7  // Pino da matriz de LEDs 5x5
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
#define BUZZER_PIN 21  // Zona morta do joystick
// Frequência do buzzer (em Hz)
// Frequência do buzzer (em Hz)
#define BUZZER_FREQUENCY 1000  // Frequência padrão do buzzer



// Ajuste do centro do joystick
#define JOYSTICK_CENTER_X 1939 // Valor de centro do eixo X
#define JOYSTICK_CENTER_Y 2180 // Valor de centro do eixo Y

// Parâmetros da tela e exibição
#define SCREEN_WIDTH 128 // Largura da tela
#define SCREEN_HEIGHT 64 // Altura da tela

// Variáveis globais
// Variáveis globais
// Variáveis globais
static volatile bool pwm_enabled = true;  // Flag para controlar o PWM
static volatile uint32_t last_button_a_time = 0;  // Controle de debounce do botão A
static volatile uint32_t last_button_joy_time = 0;  // Controle de debounce do botão do joystick

// Variável para armazenar o estado do joystick
static volatile bool joystick_activated = false;

// Variável para armazenar a quantidade de morcegos
static volatile int morcegos = 50; 

uint32_t get_time_ms(void);


// Função para gerar um número aleatório entre 10 e 100
int gerar_morcegos() {
    return (rand() % 171) + 10; // Gera um número entre 10 e 100
}


// Inicialização do PWM
void pwm_setup(uint pin) {
    gpio_set_function(pin, GPIO_FUNC_PWM);  // Configura o pino como PWM
    uint slice = pwm_gpio_to_slice_num(pin); // Obtém o slice do pino
    pwm_set_clkdiv(slice, 4.0);  // Configura o divisor de clock do PWM
    pwm_set_wrap(slice, 65535);  // Define o valor máximo do contador PWM
    pwm_set_gpio_level(pin, 0);  // Inicializa o nível do PWM em 0 (desligado)
    pwm_set_enabled(slice, true); // Habilita o PWM no slice
}

// Inicializa o PWM para o buzzer e ajusta a frequência
void pwm_buzzer_setup(uint pin, int frequency) {
    pwm_setup(pin);  // Configura o pino do buzzer como PWM
    uint slice = pwm_gpio_to_slice_num(pin);  // Obtém o slice do pino
    uint32_t pwm_clock = clock_get_hz(clk_sys);  // Obtém o clock do sistema
    uint32_t clkdiv = (float)pwm_clock / (frequency * 65536);  // Calcula o divisor de clock para a frequência desejada
    pwm_set_clkdiv(slice, clkdiv);  // Ajusta o divisor de clock
    pwm_set_wrap(slice, 65535);  // Define o valor máximo do contador PWM
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




// Definição da variável temperatura
int temperatura = 0;  // Temperatura inicial como 28°C
// Variável global para controlar se a temperatura foi fixada
static volatile bool is_temperature_locked = false;

static volatile bool is_qualidade_ar_locked = false; 

// Função de interrupção para o GPIO
static void gpio_irq_handler(uint gpio, uint32_t events) {
    if (gpio == BUTTON_A) {
        uint32_t current_time = time_us_32();
        if (current_time - last_button_a_time > 200000) { // Evita debounce
            last_button_a_time = current_time;
            is_temperature_locked = !is_temperature_locked;  // Alterna a fixação da temperatura
        }
    }
    if (gpio == BUTTON_JOY) {
        uint32_t current_time = time_us_32();
        if (current_time - last_button_joy_time > 200000) { // Evita debounce
            last_button_joy_time = current_time;
            // is_qualidade_ar_locked = !is_qualidade_ar_locked;  // Alterna a fixação da qualidade do ar
        }
    }
    if (gpio == BUTTON_B) {
        morcegos = gerar_morcegos();

        printf("Botão pressionado! Morcegos atualizados para %d\n", morcegos);
    }
}

// Atualiza a temperatura com base no movimento do joystick
// Função de atualização da temperatura com base no movimento do joystick
// Função para atualizar a temperatura com base no movimento do joystick
void update_temperature() {
    gpio_init(BUZZER_PIN);
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);

    pwm_buzzer_setup(BUZZER_PIN, BUZZER_FREQUENCY);
    uint16_t adc_y = read_adc(0);  // Lê o valor do eixo Y do joystick

    // Calcula o deslocamento do eixo Y
    int16_t offset_y = adc_y - JOYSTICK_CENTER_Y;

    // Verifica se o joystick foi movido além da zona morta
    if (abs(offset_y) > JOYSTICK_DEADZONE) {
        joystick_activated = true; // Ativa o joystick quando ele é movido
    }

    // Só altera a temperatura se o joystick foi ativado e a temperatura não estiver fixada
    if (joystick_activated && !is_temperature_locked) {
        // Se o joystick foi movido para cima (temperatura deve subir)
        if (offset_y > JOYSTICK_DEADZONE) {
            temperatura += 1;  // Aumenta a temperatura lentamente (um grau por vez)
        }
        // Se o joystick foi movido para baixo (temperatura deve diminuir)
        else if (offset_y < -JOYSTICK_DEADZONE) {
            // Tenta diminuir a temperatura
            if (temperatura > 28) {
                temperatura -= 1;  // Diminui a temperatura lentamente (um grau por vez)
            }
        }

        // Limita a temperatura dentro dos valores extremos
        if (temperatura < 10) temperatura = 10; // Garante que a temperatura não seja menor que 28
        if (temperatura > 50) temperatura = 50; // Limita o valor máximo
    }

    // Lógica para acender os LEDs conforme a temperatura
    // Se a temperatura passar de 38, acende o LED vermelho completamente
    if (temperatura > 38) {
        pwm_set_gpio_level(LED_RED, 65535);  // Acende o LED vermelho com a intensidade máxima
        gpio_put(LED_GREEN, 0);  // Apaga o LED verde
        gpio_put(LED_BLUE, 0);   // Apaga o LED azul

        pwm_set_gpio_level(BUZZER_PIN, 12767);  // Emite som médio no buzzer
        sleep_ms(500);  // Buzzer faz som por 500ms
        pwm_set_gpio_level(BUZZER_PIN, 0);  // Desliga o buzzer após 500ms
    } else if (temperatura > 34) {
        gpio_put(LED_GREEN, 1);  // Acende o LED verde
        gpio_put(LED_RED, 0);    // Apaga o LED vermelho
        gpio_put(LED_BLUE, 0);   // Apaga o LED azul

        pwm_set_gpio_level(BUZZER_PIN, 32767);  // Emite som médio no buzzer
        sleep_ms(500);  // Buzzer faz som por 500ms
        pwm_set_gpio_level(BUZZER_PIN, 0);  // Desliga o buzzer após 500ms
    } else {
        // Se a temperatura estiver abaixo de 34, apaga todos os LEDs
        gpio_put(LED_GREEN, 0);
        gpio_put(LED_RED, 0);
        gpio_put(LED_BLUE, 0);
        pwm_set_gpio_level(LED_RED, 0);
        // Desativa o buzzer
        gpio_put(BUZZER_PIN, 0);  // Desliga o buzzer
    }
}

// Definição da variável qualidade do ar
int qualidade_ar = 50;  // Qualidade inicial do ar (valor médio de 50)
int qualidade_ar_max = 100;  // Máximo de qualidade de ar
int qualidade_ar_min = 0;    // Mínimo de qualidade de ar


// Função para atualizar a qualidade do ar com base no movimento do joystick
void update_air_quality() {
    gpio_init(BUZZER_PIN);
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);
    pwm_buzzer_setup(BUZZER_PIN, BUZZER_FREQUENCY);

    uint16_t adc_x = read_adc(1);  // Lê o valor do eixo X do joystick
    int16_t offset_x = adc_x - JOYSTICK_CENTER_X; // Calcula o deslocamento do eixo X

    // Verifica se o joystick foi movido além da zona morta
    if (abs(offset_x) > JOYSTICK_DEADZONE) {
        joystick_activated = true;  // Ativa o joystick quando ele é movido
    }

    // Atualiza a qualidade do ar com base no movimento do joystick
    if (!is_qualidade_ar_locked) {
        // Se o joystick foi movido para a direita (qualidade do ar melhora)
        if (offset_x > JOYSTICK_DEADZONE) {
            qualidade_ar += 10;  // Aumenta a qualidade do ar lentamente
        }
        // Se o joystick foi movido para a esquerda (qualidade do ar piora)
        else if (offset_x < -JOYSTICK_DEADZONE) {
            if (qualidade_ar > qualidade_ar_min) {
                qualidade_ar -= 10;  // Diminui a qualidade do ar lentamente
            }
        }

        // Limita a qualidade do ar dentro dos valores extremos
        if (qualidade_ar < qualidade_ar_min) qualidade_ar = qualidade_ar_min;
        if (qualidade_ar > qualidade_ar_max) qualidade_ar = qualidade_ar_max;
    }

    // Lógica para acender LEDs baseados na qualidade do ar
    if (qualidade_ar < 50) {
        gpio_put(LED_RED, 1);  // Acende o LED vermelho
        gpio_put(LED_GREEN, 0); // Apaga o LED verde
        gpio_put(LED_BLUE, 0);  // Apaga o LED azul
        pwm_set_gpio_level(BUZZER_PIN, 1208);  // Emite som alto no buzzer
        sleep_ms(500);
        pwm_set_gpio_level(BUZZER_PIN, 0);  // Desliga o buzzer
    } else {
        gpio_put(LED_GREEN, 0);  // Apaga o LED verde
        gpio_put(LED_RED, 0);    // Apaga o LED vermelho
        gpio_put(LED_BLUE, 1);   // Acende o LED azul
        pwm_set_gpio_level(BUZZER_PIN, 0);  // Desliga o buzzer
    }
}

volatile int morcegos_detectados = 0;
volatile bool alerta_ativo = false;  // Indica se o alerta está ativo
volatile time_t tempo_inicio = 0;    // Registra o tempo inicial do alerta

// Função que verifica se os 5 segundos já passaram
void verificar_tempo_alerta() {
    if (alerta_ativo && (time(NULL) - tempo_inicio) >= 5) {
        alerta_ativo = false; // **Desativa o alerta**
        set_one_led(0, 0, 0, simbolo_perigo); // **Apaga o LED**
    }
}

// Função que inicia o alerta de piscar LEDs **somente se necessário**
void iniciar_alerta_led(int novo_numero) {
    if (novo_numero > 50 && !alerta_ativo) {  // **Só ativa se o número for maior que 50**
        alerta_ativo = true;
        tempo_inicio = time(NULL); // Armazena o tempo de início
    }
}

// Atualiza a quantidade de morcegos e gerencia o alerta
void atualizar_morcegos(int novo_numero) {
    if (novo_numero != morcegos_detectados) {
        morcegos_detectados = novo_numero;

        if (novo_numero > 50) {
            iniciar_alerta_led(novo_numero);  // **Ativa o alerta se necessário**
        } else {
            alerta_ativo = false; // **Desativa o alerta se não há perigo**
            set_one_led(0, 0, 0, simbolo_perigo); // **Apaga o LED imediatamente**
        }
    }
}

// Função para exibir a mensagem de boas-vindas
void show_welcome_message(ssd1306_t *ssd) {
    ssd1306_fill(ssd, false);  // Limpa a tela
    ssd1306_draw_string(ssd, "BEM VINDO", 25, 25);  // Exibe a mensagem de boas-vindas
    ssd1306_send_data(ssd);  // Atualiza o display

    sleep_ms(5000);  // Espera 5 segundos

    ssd1306_fill(ssd, false);  // Limpa a tela após o tempo de espera
    ssd1306_send_data(ssd);  // Atualiza o display
}

// Função de atualização do display
void update_display(ssd1306_t *ssd) {
    ssd1306_fill(ssd, false);  // Limpa a tela
    
    // Desenha a temperatura na tela
    char temp_str[16];
    snprintf(temp_str, sizeof(temp_str), "TEMP: %d C", temperatura);
    ssd1306_draw_string(ssd, temp_str, 0, 0);  // Passa o ponteiro correto e remove o 'true'

    // Desenha a qualidade do ar na tela
    char air_quality_str[16];
    snprintf(air_quality_str, sizeof(air_quality_str), "QUAL. AR: %d%%", qualidade_ar);
    ssd1306_draw_string(ssd, air_quality_str, 0, 15);

    // Desenha uma barra representando a qualidade do ar
    int air_bar_width = map_adc_to_screen(qualidade_ar, 50, 35); // Mapeia o valor para a largura da tela
    ssd1306_rect(ssd, 14, 110, air_bar_width, 10, true, true);

    
    // Atualiza a quantidade de morcegos
    char texto[20];
    sprintf(texto, "MORCEGOS: %d", morcegos);
    ssd1306_draw_string(ssd, texto, 0, 30);

    ssd1306_send_data(ssd);  // **Atualiza o display primeiro!**

    // Atualiza a contagem de morcegos e ativa/desativa o alerta
    atualizar_morcegos(morcegos);

}


// Função para exibir o alerta no display
void show_alert(ssd1306_t *ssd) {
    ssd1306_fill(ssd, false); // Limpa o display
    ssd1306_draw_string(ssd, "CONTAMINACAO", 0, 0);
    
    pwm_set_gpio_level(LED_RED, 65535);
    gpio_put(LED_GREEN, 0);
    gpio_put(LED_BLUE, 0);
    
    pwm_set_gpio_level(BUZZER_PIN, 12767);
    sleep_ms(500);
    pwm_set_gpio_level(BUZZER_PIN, 0);
    
    char alerta[64];
    snprintf(alerta, sizeof(alerta), "TEMP: %dC", temperatura);
    ssd1306_draw_string(ssd, alerta, 0, 15);
    
    snprintf(alerta, sizeof(alerta), "QUAL AR: %d", qualidade_ar);
    ssd1306_draw_string(ssd, alerta, 0, 30);
    
    snprintf(alerta, sizeof(alerta), "MORCEGOS: %d", morcegos);
    ssd1306_draw_string(ssd, alerta, 0, 45);
    
    ssd1306_send_data(ssd);
    sleep_ms(5000);
    ssd1306_fill(ssd, false); // Limpa o display ao sair do alerta
    ssd1306_send_data(ssd);
}

// Função para verificar condição de alerta
void check_alert_conditions(ssd1306_t *ssd) {
    if (temperatura > 40 && qualidade_ar < 70 && morcegos > 50) {
        show_alert(ssd);
    }
}



int main() {
    stdio_init_all();  // Inicializa a comunicação padrão
    // Variáveis e configurações PIO
    PIO pio = pio0;
    int sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);  // Adiciona o programa para controlar a matriz de LEDs
    ws2812_program_init(pio, sm, offset, MATRIZ_LED_PIN, 800000, false);  // Inicializa a matriz de LEDs

    gpio_init(BUZZER_PIN);
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);

    pwm_setup(LED_RED);  // Configura o PWM para o LED vermelho
    pwm_setup(LED_BLUE); // Configura o PWM para o LED azul

    gpio_init(LED_GREEN); // Inicializa o LED verde
    gpio_set_dir(LED_GREEN, GPIO_OUT);
    gpio_put(LED_GREEN, 0);  // Inicializa o LED verde apagado

    gpio_init(BUTTON_A);  // Inicializa o botão A
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);  // Habilita o pull-up para o botão A
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);  // Configura a interrupção para o botão A

    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B); // Ativa pull-up interno
    gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);



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

    // Exibe a mensagem de boas-vindas por 5 segundos
    show_welcome_message(&ssd);

    while (1) {
        update_temperature();      // Atualiza a temperatura
        update_air_quality();
        check_alert_conditions(&ssd);

        update_display(&ssd);    // Atualiza a qualidade do ar
        sleep_ms(100);             // Delay para o próximo ciclo
        if (alerta_ativo) {
            set_one_led(50, 0, 0, simbolo_perigo); 
            sleep_ms(500);
            set_one_led(0, 0, 0, simbolo_perigo);
            sleep_ms(500);
        }

        verificar_tempo_alerta(); // **Garante que o alerta pare após 5 segundos**
        
    }

    return 0;
}
