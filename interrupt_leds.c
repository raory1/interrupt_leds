#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/adc.h"
#include "interrupt_leds.pio.h"

// Definição dos pinos
const uint PIN_LED_RED = 13;
const uint BTN_A = 5;
const uint BTN_B = 6;

#define OUT_PIN 7

volatile bool red_led_state = false;
volatile int current_pattern = 0;
volatile absolute_time_t last_press_time = 0;

PIO pio;
uint sm;

// Matriz representando os números de 0 a 9 + estado APAGADO
double numeros[11][5][5] = {
    {
        // Apagar
        {0.0, 0.0, 0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0, 0.0, 0.0},
    },
    {
        // Número 0
        {0.0, 0.1, 0.1, 0.1, 0.0},
        {0.0, 0.1, 0.0, 0.1, 0.0},
        {0.0, 0.1, 0.0, 0.1, 0.0},
        {0.0, 0.1, 0.0, 0.1, 0.0},
        {0.0, 0.1, 0.1, 0.1, 0.0},
    },
    {
        // Número 1
        {0.0, 0.1, 0.1, 0.1, 0.0},
        {0.0, 0.0, 0.1, 0.0, 0.0},
        {0.0, 0.0, 0.1, 0.0, 0.0},
        {0.0, 0.1, 0.1, 0.0, 0.0},
        {0.0, 0.0, 0.1, 0.0, 0.0},
    },
    {
        // Número 2
        {0.0, 0.1, 0.1, 0.1, 0.0},
        {0.0, 0.1, 0.0, 0.0, 0.0},
        {0.0, 0.1, 0.1, 0.1, 0.0},
        {0.0, 0.0, 0.0, 0.1, 0.0},
        {0.0, 0.1, 0.1, 0.1, 0.0},
    },
    {
        // Número 3
        {0.0, 0.1, 0.1, 0.1, 0.0},
        {0.0, 0.0, 0.0, 0.1, 0.0},
        {0.0, 0.1, 0.1, 0.0, 0.0},
        {0.0, 0.0, 0.0, 0.1, 0.0},
        {0.0, 0.1, 0.1, 0.1, 0.0},
    },
    {
        // Número 4
        {0.0, 0.1, 0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0, 0.1, 0.0},
        {0.0, 0.1, 0.1, 0.1, 0.0},
        {0.0, 0.1, 0.0, 0.1, 0.0},
        {0.0, 0.1, 0.0, 0.1, 0.0},
    },
    {
        // Número 5
        {0.0, 0.1, 0.1, 0.1, 0.0},
        {0.0, 0.0, 0.0, 0.1, 0.0},
        {0.0, 0.1, 0.1, 0.1, 0.0},
        {0.0, 0.1, 0.0, 0.0, 0.0},
        {0.0, 0.1, 0.1, 0.1, 0.0},
    },
    {
        // Número 6
        {0.0, 0.1, 0.1, 0.1, 0.0},
        {0.0, 0.1, 0.0, 0.1, 0.0},
        {0.0, 0.1, 0.1, 0.1, 0.0},
        {0.0, 0.1, 0.0, 0.0, 0.0},
        {0.0, 0.1, 0.1, 0.1, 0.0},
    },
    {
        // Número 7
        {0.0, 0.1, 0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0, 0.1, 0.0},
        {0.0, 0.1, 0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0, 0.1, 0.0},
        {0.0, 0.1, 0.1, 0.1, 0.0},
    },
    {
        // Número 8
        {0.0, 0.1, 0.1, 0.1, 0.0},
        {0.0, 0.1, 0.0, 0.1, 0.0},
        {0.0, 0.1, 0.1, 0.1, 0.0},
        {0.0, 0.1, 0.0, 0.1, 0.0},
        {0.0, 0.1, 0.1, 0.1, 0.0},
    },
    {
        // Número 9
        {0.0, 0.1, 0.1, 0.1, 0.0},
        {0.0, 0.0, 0.0, 0.1, 0.0},
        {0.0, 0.1, 0.1, 0.1, 0.0},
        {0.0, 0.1, 0.0, 0.1, 0.0},
        {0.0, 0.1, 0.1, 0.1, 0.0},
    },
};

uint32_t matrix_rgb(double r, double g, double b)
{
    return ((uint8_t)(g * 255) << 24) | ((uint8_t)(r * 255) << 16) | ((uint8_t)(b * 255) << 8);
}

// Atualiza os LEDs da matriz para exibir o padrão atual
void atualizar_matriz_leds(PIO pio, uint sm, int current_pattern)
{
    for (int j = 0; j < 5; j++)
    {
        for (int k = 0; k < 5; k++)
        {
            uint32_t cor = matrix_rgb(numeros[current_pattern][j][k], 0, 0);
            pio_sm_put_blocking(pio, sm, cor);
        }
    }
}

// Função de interrupção para os botões
void gpio_irq_handler(uint gpio, uint32_t events)
{
    bool btn_last_state = false;

    uint32_t current_time = to_us_since_boot(get_absolute_time());
    bool btn_pressed = !gpio_get(gpio);

    // Condição para evitar repetições de cliques rápidas (bouncing)
    if (btn_pressed && !btn_last_state &&
        (absolute_time_diff_us(last_press_time, get_absolute_time()) > 200000))
    {
        last_press_time = get_absolute_time();
        btn_last_state = true;
        // Se o botão A for pressionado, decrementa o padrão (limite mínimo 0)
        if (gpio == BTN_A && current_pattern < 10)
        {
            printf("Botão A pressionado\n");
            current_pattern++;
            atualizar_matriz_leds(pio, sm, current_pattern);
        }
        // Se o botão B for pressionado, incrementa o padrão (limite máximo 10)
        else if (gpio == BTN_B && current_pattern > 0)
        {
            printf("Botão B pressionado\n");
            current_pattern--;
            atualizar_matriz_leds(pio, sm, current_pattern);
        }
    }

    else if (!btn_pressed)
    {
        btn_last_state = false;
    }
}

// Função para alternar o estado de um LED.
bool blink_led(struct repeating_timer *t)
{
    red_led_state = !red_led_state;
    gpio_put(PIN_LED_RED, red_led_state);
    return true;
}

int main()
{
    pio = pio0;
    set_sys_clock_khz(128000, false);
    stdio_init_all();

    uint offset = pio_add_program(pio, &pio_matrix_program);
    sm = pio_claim_unused_sm(pio, true);
    pio_matrix_program_init(pio, sm, offset, OUT_PIN);

    // Inicialização e configuração do LED
    gpio_init(PIN_LED_RED);
    gpio_set_dir(PIN_LED_RED, GPIO_OUT);

    // Inicialização e configuração dos botões
    gpio_init(BTN_A);
    gpio_set_dir(BTN_A, GPIO_IN);
    gpio_pull_up(BTN_A);
    gpio_init(BTN_B);
    gpio_set_dir(BTN_B, GPIO_IN);
    gpio_pull_up(BTN_B);

    // Habilita interrupções para os botões
    gpio_set_irq_enabled_with_callback(BTN_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(BTN_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    atualizar_matriz_leds(pio, sm, current_pattern);

    // Invoca a função blink_led através de um timer repetitivo a cada 100ms
    struct repeating_timer timer;
    add_repeating_timer_ms(-100, blink_led, NULL, &timer);

    // Loop principal
    while (true)
    {
        tight_loop_contents(); // Impede possíveis otimizações indesejáveis
    }
}