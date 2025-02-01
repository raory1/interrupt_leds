#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/adc.h"
#include "interrupt_leds.pio.h"

// Definição dos pinos
const uint PIN_LED_RED = 13;
const uint PIN_LED_BLUE = 12;
const uint BTN_A = 5;
const uint BTN_B = 6;

#define NUM_PIXELS 25
#define OUT_PIN 7
volatile int current_pattern = 0;

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
        // Número 8
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

void acender_todos_leds(PIO pio, uint sm, int current_pattern)
{
    for (int j = 0; j < 5; j++)
    {
        for (int k = 0; k < 5; k++)
        {
            uint32_t cor = matrix_rgb(numeros[current_pattern][j][k], 0, 0);
            pio_sm_put_blocking(pio, sm, cor);
        }
    }
    sleep_ms(100);
}

void gpio_irq_handler(uint gpio, uint32_t events)
{
    if (gpio_get(BTN_A))
    {
        bool estado_atual = gpio_get(PIN_LED_BLUE);
        gpio_put(PIN_LED_BLUE, !estado_atual);
        current_pattern--;
    }

    else
    {
        bool estado_atual = gpio_get(PIN_LED_BLUE);
        gpio_put(PIN_LED_BLUE, !estado_atual);
        current_pattern++;
    }
}

int main()
{
    PIO pio = pio0;
    set_sys_clock_khz(128000, false);
    stdio_init_all();

    uint offset = pio_add_program(pio, &pio_matrix_program);
    uint sm = pio_claim_unused_sm(pio, true);
    pio_matrix_program_init(pio, sm, offset, OUT_PIN);

    stdio_init_all();
    // Inicialização e configuração do LED
    gpio_init(PIN_LED_RED);
    gpio_set_dir(PIN_LED_RED, GPIO_OUT);
    gpio_init(PIN_LED_BLUE);
    gpio_set_dir(PIN_LED_BLUE, GPIO_OUT);

    // Inicialização e configuração dos botões
    gpio_init(BTN_A);
    gpio_set_dir(BTN_A, GPIO_IN);
    gpio_pull_up(BTN_A);
    gpio_init(BTN_B);
    gpio_set_dir(BTN_B, GPIO_IN);
    gpio_pull_up(BTN_B);

    gpio_set_irq_enabled_with_callback(BTN_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(BTN_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    // Loop principal, LED vermelho pisca constantemente
    while (true)
    {
        acender_todos_leds(pio, sm, current_pattern);

        gpio_put(PIN_LED_RED, false);
        sleep_ms(100);
        //gpio_put(PIN_LED_RED, true);
        sleep_ms(100);
    }
}
